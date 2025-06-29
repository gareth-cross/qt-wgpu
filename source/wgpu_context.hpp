#pragma once
#include <webgpu/webgpu_cpp.h>

namespace wgpu_utils {

// Store the device and information about the render surface.
class wgpu_context {
 public:
  explicit wgpu_context(wgpu::Instance instance, wgpu::Surface surface);

  constexpr const auto& surface() const noexcept { return surface_; }
  constexpr const auto& device() const noexcept { return device_; }

  constexpr std::optional<wgpu::TextureFormat> surface_format() const noexcept { return surface_format_; }

  void configure_surface(std::uint32_t width, std::uint32_t height);

 private:
  wgpu::Adapter adapter_;
  wgpu::Device device_;
  wgpu::Surface surface_;
  std::optional<wgpu::TextureFormat> surface_format_;
};

}  // namespace wgpu_utils
