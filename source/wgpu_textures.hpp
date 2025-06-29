#pragma once
#include <webgpu/webgpu_cpp.h>

namespace wgpu_utils {

// Create texture suitable for MSAA render attachment.
wgpu::Texture create_multisample_texure(const wgpu::Device& device, const wgpu::TextureFormat texture_format,
                                        std::uint32_t width, std::uint32_t height, std::uint32_t multisample_count);

// Get the next texture view in the swap chain for our target surface.
wgpu::TextureView get_next_surface_texture_view(const wgpu::Device& device, const wgpu::Surface& surface);

// Create a 32-bit texture suitable for a depth buffer.
wgpu::Texture create_depth_texture(const wgpu::Device& device, std::uint32_t width, std::uint32_t height,
                                   std::uint32_t multisample_count);

}  // namespace wgpu_utils
