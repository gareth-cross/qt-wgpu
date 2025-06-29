#include "wgpu_textures.hpp"

#include <qassert.h>

#include "wgpu_error_scope.hpp"

namespace wgpu_utils {

wgpu::Texture create_multisample_texure(const wgpu::Device& device, const wgpu::TextureFormat texture_format,
                                        std::uint32_t width, std::uint32_t height, std::uint32_t sample_count) {
  WGPU_ERROR_FUNCTION_SCOPE(device);

  wgpu::TextureDescriptor texture_descriptor{};
  texture_descriptor.format = texture_format;
  texture_descriptor.usage = wgpu::TextureUsage::RenderAttachment;
  texture_descriptor.size = wgpu::Extent3D{std::max(width, 1u), std::max(height, 1u)};
  texture_descriptor.sampleCount = sample_count;
  return device.CreateTexture(&texture_descriptor);
}

wgpu::TextureView get_next_surface_texture_view(const wgpu::Device& device, const wgpu::Surface& surface) {
  WGPU_ERROR_FUNCTION_SCOPE(device);

  // Get the surface texture
  wgpu::SurfaceTexture surface_texture;
  surface.GetCurrentTexture(&surface_texture);
  Q_ASSERT(surface_texture.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal ||
           surface_texture.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal);

  // Create a view for this surface texture
  wgpu::TextureViewDescriptor view_descriptor{};
  view_descriptor.label = "Surface texture view";
  view_descriptor.format = surface_texture.texture.GetFormat();
  view_descriptor.dimension = wgpu::TextureViewDimension::e2D;
  view_descriptor.baseMipLevel = 0;
  view_descriptor.mipLevelCount = 1;
  view_descriptor.aspect = wgpu::TextureAspect::All;
  return surface_texture.texture.CreateView(&view_descriptor);
}

wgpu::Texture create_depth_texture(const wgpu::Device& device, std::uint32_t width, std::uint32_t height,
                                   std::uint32_t sample_count) {
  WGPU_ERROR_FUNCTION_SCOPE(device);

  wgpu::TextureDescriptor texture_descriptor{};
  texture_descriptor.label = "Depth texture";
  texture_descriptor.size = wgpu::Extent3D{std::max(width, 1u), std::max(height, 1u), 1};
  texture_descriptor.mipLevelCount = 1;
  texture_descriptor.sampleCount = sample_count;
  texture_descriptor.format = wgpu::TextureFormat::Depth32Float;
  texture_descriptor.dimension = wgpu::TextureDimension::e2D;
  texture_descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
  return device.CreateTexture(&texture_descriptor);
}

}  // namespace wgpu_utils
