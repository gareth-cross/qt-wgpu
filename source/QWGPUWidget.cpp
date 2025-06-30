#include "QWGPUWidget.h"

#include <QCoreApplication>

#include "wgpu_error_scope.hpp"
#include "wgpu_textures.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>  // GetModuleHandle

// Get a surface descriptor from windows HWND.
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> CreateSurfaceDescriptor(QWidget* const widget) {
  wgpu::SurfaceSourceWindowsHWND* desc = new wgpu::SurfaceSourceWindowsHWND();
  desc->hwnd = reinterpret_cast<HWND>(widget->winId());
  desc->hinstance = GetModuleHandle(nullptr);
  return {desc, [](wgpu::ChainedStruct* desc) { delete static_cast<wgpu::SurfaceSourceWindowsHWND*>(desc); }};
}
#elif defined(__APPLE__)

// Defined in create_surface_descriptor.mm
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> CreateSurfaceDescriptor(QWidget* const widget);

#elif defined(__linux__)
// We need the private headers from QPlatformNativeInterface on linux.
#include <QtGui/6.9.1/QtGui/qpa/qplatformnativeinterface.h>
#include <QGuiApplication>

std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> CreateSurfaceDescriptor(QWidget* const widget) {
  wgpu::SurfaceSourceWaylandSurface* desc = new wgpu::SurfaceSourceWaylandSurface();
  QPlatformNativeInterface* const platformNativeInterface = QGuiApplication::platformNativeInterface();
  Q_ASSERT(platformNativeInterface);
  desc->display = platformNativeInterface->nativeResourceForIntegration("wl_display");
  desc->surface = platformNativeInterface->nativeResourceForWindow("surface", widget->windowHandle());
  qInfo("Got wayland display %p and surface %p", desc->display, desc->surface);
  return {desc, [](wgpu::ChainedStruct* desc) { delete static_cast<wgpu::SurfaceSourceWaylandSurface*>(desc); }};
}
#else
#error Platform not supported.
#endif

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

  // Required for non-transparent status bar on linux:
  QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

  setFocusPolicy(Qt::StrongFocus);
  // NOTE(gareth): Unclear if WA_DontCreateNativeAncestors is strictly required here.
  setAttribute(Qt::WA_DontCreateNativeAncestors);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
}

void QWGPUWidget::run() {
  // Trigger rendering at ~60Hz.
  connect(&frame_timer_, &QTimer::timeout, this, &QWGPUWidget::onFrameTimerFired);
  frame_timer_.start(16);
  start_time_ = std::chrono::steady_clock::now();
}

// Start a render pass by clearing depth + RGB.
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

static constexpr std::string_view shader_source_code = R"wgsl(
// WGPU NDC coordinates are +Y goes up, +X goes right.
const p_normalized: array<vec2f, 4> = array<vec2f, 4>(
  vec2f(-1.0, -1.0),  // NDC bottom left
  vec2f( 1.0, -1.0),  // NDC bottom right
  vec2f( 1.0,  1.0),  // NDC top right
  vec2f(-1.0,  1.0)   // NDC top left
);

// WGPU texture coordinates have x-right y-down.
const uvs: array<vec2f, 4> = array<vec2f, 4>(
  vec2f(0.0, 1.0),
  vec2f(1.0, 1.0),
  vec2f(1.0, 0.0),
  vec2f(0.0, 0.0)
);

const colors: array<vec3f, 4> = array<vec3f, 4>(
  vec3f(1.0, 0.0, 0.0),
  vec3f(0.0, 1.0, 0.0),
  vec3f(0.0, 0.0, 1.0),
  vec3f(1.0, 1.0, 1.0),
);

const vertex_indices: array<u32, 6> = array<u32, 6>(0, 1, 2, 2, 3, 0);

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) color: vec3f,
};

@group(0) @binding(0) var<uniform> time: f32;

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
  let index: u32 = vertex_indices[in_vertex_index];

  // Rotate the quad as time elapses:
  let angle = 0.2 * time;
  let p: vec2f = p_normalized[index] * 0.5;
  let p_rotated = mat2x2f(cos(angle), sin(angle), -sin(angle), cos(angle)) * p;

  var out: VertexOutput;
  out.position = vec4f(p_rotated, 0.0, 1.0);
  out.uv = uvs[index];
  out.color = colors[index];
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  return vec4f(in.color, 1.0);
}
)wgsl";

// Create a simple pipeline that draws a quad on screen.
std::tuple<wgpu::RenderPipeline, wgpu::BindGroupLayout> make_toy_render_pipeline(
    const wgpu::Device& device, const wgpu::TextureFormat surface_format, const std::uint32_t multisample_count) {
  WGPU_ERROR_FUNCTION_SCOPE(device);

  // Compile simple shader:
  wgpu::ShaderModuleDescriptor shader_desc{};
  wgpu::ShaderSourceWGSL shader_source{};
  shader_desc.nextInChain = &shader_source;
  shader_source.code = shader_source_code;
  const auto shader = device.CreateShaderModule(&shader_desc);

  wgpu::BindGroupLayoutEntry entry{};
  entry.binding = 0;
  entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  entry.buffer.type = wgpu::BufferBindingType::Uniform;
  entry.buffer.minBindingSize = 16;  // Rounded for alignment.

  wgpu::BindGroupLayoutDescriptor descriptor{};
  descriptor.entryCount = 1;
  descriptor.entries = &entry;
  descriptor.label = "Bind group layout";

  const auto bg_layout = device.CreateBindGroupLayout(&descriptor);

  wgpu::PipelineLayoutDescriptor pipeline_layout_desc{};
  pipeline_layout_desc.label = "Pipeline layout";
  pipeline_layout_desc.bindGroupLayoutCount = 1;
  pipeline_layout_desc.bindGroupLayouts = &bg_layout;
  const auto pipeline_layout = device.CreatePipelineLayout(&pipeline_layout_desc);

  wgpu::FragmentState frag_state{};
  frag_state.module = shader;
  frag_state.entryPoint = "fs_main";

  wgpu::BlendState blend_state{};
  blend_state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
  blend_state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
  blend_state.color.operation = wgpu::BlendOperation::Add;
  blend_state.alpha.srcFactor = wgpu::BlendFactor::Zero;
  blend_state.alpha.dstFactor = wgpu::BlendFactor::One;
  blend_state.alpha.operation = wgpu::BlendOperation::Add;

  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = surface_format;
  color_target_state.blend = &blend_state;
  color_target_state.writeMask = wgpu::ColorWriteMask::All;
  frag_state.targetCount = 1;
  frag_state.targets = &color_target_state;

  wgpu::DepthStencilState depth_state{};
  depth_state.format = wgpu::TextureFormat::Depth32Float;
  depth_state.depthWriteEnabled = true;
  depth_state.depthCompare = wgpu::CompareFunction::Less;

  wgpu::RenderPipelineDescriptor pipeline_descriptor{};
  pipeline_descriptor.vertex.module = shader;
  pipeline_descriptor.vertex.entryPoint = "vs_main";
  pipeline_descriptor.vertex.bufferCount = 0;

  pipeline_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  pipeline_descriptor.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
  pipeline_descriptor.primitive.frontFace = wgpu::FrontFace::CCW;
  pipeline_descriptor.primitive.cullMode = wgpu::CullMode::Back;
  pipeline_descriptor.fragment = &frag_state;
  pipeline_descriptor.depthStencil = &depth_state;
  pipeline_descriptor.multisample.count = multisample_count;
  pipeline_descriptor.multisample.mask = ~0u;
  pipeline_descriptor.multisample.alphaToCoverageEnabled = false;
  pipeline_descriptor.label = "Toy pipeline";
  pipeline_descriptor.layout = pipeline_layout;
  const auto pipeline = device.CreateRenderPipeline(&pipeline_descriptor);
  return std::make_tuple(pipeline, bg_layout);
}

void QWGPUWidget::onFrameTimerFired() {
  WGPU_ERROR_FUNCTION_SCOPE(context_->device());

  // Only value supported by WGPU, AFAIK.
  constexpr std::uint32_t sample_count = 4;

  if (width_ != this->width() || height_ != this->height()) {
    // TODO: These dimensions probably don't account for retina displays on mac.
    width_ = this->width();
    height_ = this->height();
    context_->configure_surface(static_cast<std::uint32_t>(width_), static_cast<std::uint32_t>(height_));

    // Create a color + depth texture suitable for MSAA rendering.
    msaa_texture_ = wgpu_utils::create_multisample_texure(context_->device(), context_->surface_format().value(),
                                                          static_cast<std::uint32_t>(width_),
                                                          static_cast<std::uint32_t>(height_), sample_count);

    depth_texture_ = wgpu_utils::create_depth_texture(context_->device(), static_cast<std::uint32_t>(width_),
                                                      static_cast<std::uint32_t>(height_), sample_count);

    qInfo("Configured surface: %i x %i (window size is %i x %i)", width_, height_, this->window()->width(),
          this->window()->height());
  }

  if (!pipeline_) {
    qInfo("Creating render pipeline...");
    std::tie(pipeline_, bg_layout_) =
        make_toy_render_pipeline(context_->device(), context_->surface_format().value(), sample_count);

    wgpu::BufferDescriptor descriptor{};
    descriptor.size = 16;
    descriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    descriptor.label = "Uniform buffer";
    uniform_buffer_ = context_->device().CreateBuffer(&descriptor);
  }

  // Get a texture view for our target surface:
  auto target_view = wgpu_utils::get_next_surface_texture_view(context_->device(), context_->surface());
  Q_ASSERT(target_view);

  // Create a bundle encoder so we can do multi-sampled rendering:
  const auto surface_format = context_->surface_format().value();
  wgpu::RenderBundleEncoderDescriptor encoder_desc{};
  encoder_desc.sampleCount = sample_count;
  encoder_desc.colorFormatCount = 1;
  encoder_desc.colorFormats = &surface_format;
  encoder_desc.label = "Bundle encoder";
  encoder_desc.depthStencilFormat = wgpu::TextureFormat::Depth32Float;
  const wgpu::RenderBundleEncoder bundle_encoder = context_->device().CreateRenderBundleEncoder(&encoder_desc);
  Q_ASSERT(bundle_encoder);

  const wgpu::Queue queue = context_->device().GetQueue();
  Q_ASSERT(queue);

  // Draw our toy pipeline:
  // First update the uniform value with elapsed time.
  const auto time_elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time_.value());
  const float buffer_values[4] = {static_cast<float>(time_elapsed.count()) / 1.0e6f, 0.0f, 0.0f, 0.0f};
  queue.WriteBuffer(uniform_buffer_, 0, &buffer_values, sizeof(buffer_values));

  // Create bind group with our uniform buffer.
  wgpu::BindGroupEntry binding{};
  binding.binding = 0;
  binding.buffer = uniform_buffer_;
  binding.offset = 0;
  binding.size = sizeof(buffer_values);
  wgpu::BindGroupDescriptor bind_group_desc{};
  bind_group_desc.layout = bg_layout_;
  bind_group_desc.entryCount = 1;
  bind_group_desc.entries = &binding;
  const auto bg = context_->device().CreateBindGroup(&bind_group_desc);

  // Draw the quad...
  bundle_encoder.SetPipeline(pipeline_);
  bundle_encoder.SetBindGroup(0, bg, 0, nullptr);
  bundle_encoder.Draw(6, 1, 0, 0);

  wgpu::RenderBundleDescriptor out{};
  const auto bundle = bundle_encoder.Finish(&out);
  Q_ASSERT(bundle);

  wgpu::CommandEncoderDescriptor command_encoder_desc{};
  const auto command_encoder = context_->device().CreateCommandEncoder(&command_encoder_desc);
  Q_ASSERT(command_encoder);

  // Execute the render bundle and submit to the command queue:
  const auto render_pass_encoder = make_render_pass_encoder_with_targets(command_encoder, target_view, msaa_texture_,
                                                                         depth_texture_, "Main render pass");
  Q_ASSERT(render_pass_encoder);

  render_pass_encoder.ExecuteBundles(1, &bundle);
  render_pass_encoder.End();

  wgpu::CommandBufferDescriptor cmd_buffer_descriptor{};
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

void QWGPUWidget::resizeEvent(QResizeEvent*) {
  if (context_) {
    onFrameTimerFired();
  }
}
