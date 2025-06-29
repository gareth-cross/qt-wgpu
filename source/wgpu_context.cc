#include "wgpu_context.hpp"

#include <qassert.h>
#include <span>

#include "wgpu_error_scope.hpp"
#include "wgpu_fmt.hpp"
#include "wgpu_setup.hpp"

namespace wgpu_utils {

wgpu_context::wgpu_context(wgpu::Instance instance, wgpu::Surface surface) : surface_(surface) {
  Q_ASSERT(instance);

  adapter_ = wgpu_utils::request_adapter(instance);
  Q_ASSERT(adapter_);
  enumerate_adapter_properties(adapter_);
  enumerate_adapter_features(adapter_);

  device_ = wgpu_utils::request_device(adapter_);
  Q_ASSERT(device_);
  enumerate_device_limits(device_);

  if (surface_) {
    wgpu::SurfaceCapabilities capabilities{};
    surface_.GetCapabilities(adapter_, &capabilities);

    const std::span<const wgpu::TextureFormat> supported_formats{capabilities.formats, capabilities.formatCount};
    fmt::print("Supported texture formats:\n");
    for (const auto format : supported_formats) {
      fmt::print(" - {}\n", fmt_enum(format));
    }
    Q_ASSERT(!supported_formats.empty());
    surface_format_ = supported_formats.front();
  }
}

void wgpu_context::configure_surface(std::uint32_t width, std::uint32_t height) {
  Q_ASSERT(surface_);
  WGPU_ERROR_FUNCTION_SCOPE(device_);

  wgpu::SurfaceConfiguration config{};
  config.width = width;
  config.height = height;
  config.usage = wgpu::TextureUsage::RenderAttachment;
  config.format = surface_format_.value();
  config.device = device_;
  config.presentMode = wgpu::PresentMode::Fifo;
  config.alphaMode = wgpu::CompositeAlphaMode::Auto;
  surface_.Configure(&config);
}

}  // namespace wgpu_utils
