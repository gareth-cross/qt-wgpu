#include <webgpu/webgpu_cpp.h>

namespace wgpu_utils {

wgpu::Adapter request_adapter(const wgpu::Instance& instance);

wgpu::Device request_device(const wgpu::Adapter& adapter);

// Enumerate and print adapter features.
void enumerate_adapter_features(const wgpu::Adapter& adapter);
void enumerate_adapter_properties(const wgpu::Adapter& adapter);

// Inspect and print device spec.
void enumerate_device_limits(const wgpu::Device& device);

}  // namespace wgpu_utils
