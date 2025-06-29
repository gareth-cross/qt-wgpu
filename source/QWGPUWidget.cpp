#include "QWGPUWidget.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "wgpu_error_scope.hpp"
#include "wgpu_textures.hpp"

std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> CreateSurfaceDescriptor(QWidget* const widget) {
  wgpu::SurfaceSourceWindowsHWND* desc = new wgpu::SurfaceSourceWindowsHWND();
  desc->hwnd = static_cast<HWND>(reinterpret_cast<void*>(widget->winId()));
  desc->hinstance = GetModuleHandle(nullptr);
  return {desc, [](wgpu::ChainedStruct* desc) { delete static_cast<wgpu::SurfaceSourceWindowsHWND*>(desc); }};
}

#endif  // _WIN32

wgpu::Surface CreateSurfaceForWidget(const wgpu::Instance& instance, QWidget* const widget) {
  auto chained_descriptor = CreateSurfaceDescriptor(widget);
  wgpu::SurfaceDescriptor descriptor{};
  descriptor.nextInChain = chained_descriptor.get();
  return instance.CreateSurface(&descriptor);
}

QWGPUWidget::QWGPUWidget(QWidget* parent) : QWidget(parent) {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::black);
  setAutoFillBackground(true);
  setPalette(pal);

  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
}

QWGPUWidget::~QWGPUWidget() {}

void QWGPUWidget::run() {
  connect(&frame_timer_, &QTimer::timeout, this, &QWGPUWidget::onFrameTimerFired);
  frame_timer_.start(33);
}

wgpu::RenderPassEncoder make_render_pass_encoder_with_targets(const wgpu::CommandEncoder& encoder,
                                                              const wgpu::TextureView& target_texture_view,
                                                              const wgpu::Texture& msaa_color_texture,
                                                              const wgpu::Texture& depth_texture,
                                                              const std::string_view label) {
  wgpu::RenderPassColorAttachment render_pass_color_attachment{};
  if (msaa_color_texture) {
    render_pass_color_attachment.view = msaa_color_texture.CreateView();
    render_pass_color_attachment.resolveTarget = target_texture_view;
  } else {
    render_pass_color_attachment.view = target_texture_view;
  }
  Q_ASSERT(render_pass_color_attachment.view);

  render_pass_color_attachment.loadOp = wgpu::LoadOp::Clear;
  render_pass_color_attachment.storeOp = wgpu::StoreOp::Store;
  render_pass_color_attachment.clearValue = wgpu::Color{0.235, 0.235, 0.235, 1.0};
  render_pass_color_attachment.depthSlice = wgpu::kDepthSliceUndefined;

  wgpu::RenderPassDepthStencilAttachment depth_stencil_attachment{};
  if (depth_texture) {
    depth_stencil_attachment.view = depth_texture.CreateView();
    depth_stencil_attachment.depthLoadOp = wgpu::LoadOp::Clear;
    depth_stencil_attachment.depthStoreOp = wgpu::StoreOp::Store;
    depth_stencil_attachment.depthClearValue = 1.0f;
  }

  wgpu::RenderPassDescriptor render_pass_descriptor{};
  render_pass_descriptor.label = label;
  render_pass_descriptor.colorAttachmentCount = 1;
  render_pass_descriptor.colorAttachments = &render_pass_color_attachment;
  render_pass_descriptor.depthStencilAttachment = depth_texture ? &depth_stencil_attachment : nullptr;

  return encoder.BeginRenderPass(&render_pass_descriptor);
}

void QWGPUWidget::onFrameTimerFired() {
  WGPU_ERROR_FUNCTION_SCOPE(context_->device());

  if (width_ != this->width() || height_ != this->height()) {
    width_ = this->width();
    height_ = this->height();
    context_->configure_surface(static_cast<std::uint32_t>(width_), static_cast<std::uint32_t>(height_));

    msaa_texture_ = wgpu_utils::create_multisample_texure(*context_, static_cast<std::uint32_t>(width_),
                                                          static_cast<std::uint32_t>(height_), 4);

    depth_texture_ = wgpu_utils::create_depth_texture(context_->device(), static_cast<std::uint32_t>(width_),
                                                      static_cast<std::uint32_t>(height_), 4);

    qInfo("Configured surface: %i x %i", width_, height_);
  }

  // Get a texture view for our target surface:
  auto target_view = wgpu_utils::get_next_surface_texture_view(context_->device(), context_->surface());
  Q_ASSERT(target_view);

  const auto surface_format = context_->surface_format().value();
  wgpu::RenderBundleEncoderDescriptor encoder_desc{};
  encoder_desc.sampleCount = 4;
  encoder_desc.colorFormatCount = 1;
  encoder_desc.colorFormats = &surface_format;
  encoder_desc.label = "Bundle encoder";
  encoder_desc.depthStencilFormat = wgpu::TextureFormat::Depth32Float;
  wgpu::RenderBundleEncoder bundle_encoder = context_->device().CreateRenderBundleEncoder(&encoder_desc);
  Q_ASSERT(bundle_encoder);

  wgpu::Queue queue = context_->device().GetQueue();
  Q_ASSERT(queue);

  wgpu::RenderBundleDescriptor out{};
  out.label = "Main render bundle descriptor";
  const auto bundle = bundle_encoder.Finish(&out);
  Q_ASSERT(bundle);

  wgpu::CommandEncoderDescriptor command_encoder_desc{};
  command_encoder_desc.label = "Main command encoder";
  auto command_encoder = context_->device().CreateCommandEncoder(&command_encoder_desc);
  Q_ASSERT(command_encoder);

  // Execute the render bundle and submit to the command queue:
  const auto render_pass_encoder = make_render_pass_encoder_with_targets(command_encoder, target_view, msaa_texture_,
                                                                         depth_texture_, "Main render pass");
  Q_ASSERT(render_pass_encoder);

  render_pass_encoder.ExecuteBundles(1, &bundle);
  render_pass_encoder.End();

  wgpu::CommandBufferDescriptor cmd_buffer_descriptor{};
  cmd_buffer_descriptor.label = "Command buffer";
  const wgpu::CommandBuffer command = command_encoder.Finish(&cmd_buffer_descriptor);

  queue.Submit(1, &command);

  context_->surface().Present();
  context_->device().Tick();
}

QPaintEngine* QWGPUWidget::paintEngine() const { return nullptr; }

void QWGPUWidget::paintEvent(QPaintEvent*) {}

void QWGPUWidget::showEvent(QShowEvent* event) {
  if (!context_) {
    qInfo("Creating wgpu instance...");
    const wgpu::InstanceDescriptor desc{};
    const auto instance = wgpu::CreateInstance(&desc);
    Q_ASSERT(instance);

    qInfo("Creating wgpu surface...");
    const auto surface = CreateSurfaceForWidget(instance, this);
    Q_ASSERT(surface);

    qInfo("Requesting adapter and device...");
    context_ = wgpu_utils::wgpu_context(instance, surface);

    emit deviceInitialized();
  }
  QWidget::showEvent(event);
}
