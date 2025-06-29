#include "wgpu_setup.hpp"

#include <span>
#include <sstream>

#include "wgpu_fmt.hpp"

namespace wgpu_utils {

wgpu::Adapter request_adapter(const wgpu::Instance& instance) {
  bool request_ended = false;
  wgpu::Adapter adapter_out{};

  wgpu::RequestAdapterOptions options{};
  options.powerPreference = wgpu::PowerPreference::HighPerformance;
  instance.RequestAdapter(&options, wgpu::CallbackMode::AllowSpontaneous,
                          [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
                            if (status == wgpu::RequestAdapterStatus::Success) {
                              adapter_out = std::move(adapter);
                            } else {
                              fmt::print("Failed to get wgpu adapter. Reason: {}", message);
                            }
                            request_ended = true;
                          });
  while (!request_ended) {
#if defined(EMSCRIPTEN)
    emscripten_sleep(100);
#endif
  }
  return adapter_out;
}

wgpu::Device request_device(const wgpu::Adapter& adapter) {
  wgpu::DeviceDescriptor device_descriptor{};
  device_descriptor.label = "Default device";
  device_descriptor.requiredLimits = nullptr;
  device_descriptor.defaultQueue.label = "Default queue";
  device_descriptor.SetDeviceLostCallback(
      wgpu::CallbackMode::AllowSpontaneous,
      [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
        fmt::print("Device lost [reason: {}]. Message: {}\n", fmt_enum(reason), message);
      });

  wgpu::Device device_out{};
  bool request_ended{false};
  adapter.RequestDevice(&device_descriptor, wgpu::CallbackMode::AllowSpontaneous,
                        [&](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
                          if (status == wgpu::RequestDeviceStatus::Success) {
                            device_out = std::move(device);
                          } else {
                            fmt::print("Could get wgpu device. Reason: {}\n", message);
                          }
                          request_ended = true;
                        });

  while (!request_ended) {
#if defined(EMSCRIPTEN)
    emscripten_sleep(100);
#endif
  }

  if (device_out) {
    device_out.SetLoggingCallback([](wgpu::LoggingType log_type, wgpu::StringView message) {
      fmt::print("wgpu [{}]: {}", fmt_enum(log_type), message);
    });
  }

  return device_out;
}

void enumerate_adapter_features(const wgpu::Adapter& adapter) {
  wgpu::SupportedFeatures features{};
  adapter.GetFeatures(&features);

  const std::span<const wgpu::FeatureName> feature_span(features.features, features.featureCount);

  fmt::print("Adapter features:\n");
  for (const auto f : feature_span) {
    fmt::print(" - {}\n", fmt_enum(f));
  }
}

void enumerate_adapter_properties(const wgpu::Adapter& adapter) {
  wgpu::AdapterInfo info{};
  const bool got_adapter_info = adapter.GetInfo(&info);

  constexpr auto msg = R"(Adapter properties:
 - vendorID: {:#x}
 - vendor: {}
 - architecture: {}
 - deviceID: {:#x}
 - device: {}
 - description: {}
 - adapterType: {}
 - backendType: {}
)";
  fmt::print(msg, info.vendorID, info.vendor, info.architecture, info.deviceID, info.device, info.description,
             fmt_enum(info.adapterType), fmt_enum(info.backendType));
}

void enumerate_device_limits(const wgpu::Device& device) {
  wgpu::SupportedFeatures features{};
  device.GetFeatures(&features);

  fmt::print("Device features:\n");
  for (auto f : std::span<const wgpu::FeatureName>(features.features, features.featureCount)) {
    fmt::print(" - {}\n", fmt_enum(f));
  }

  wgpu::Limits limits{};
  std::stringstream out{};
  if (device.GetLimits(&limits)) {
    out << "Device limits:" << std::endl;
    out << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
    out << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
    out << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
    out << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    out << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
    out << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.maxDynamicUniformBuffersPerPipelineLayout
        << std::endl;
    out << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.maxDynamicStorageBuffersPerPipelineLayout
        << std::endl;
    out << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage << std::endl;
    out << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
    out << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << std::endl;
    out << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage << std::endl;
    out << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << std::endl;
    out << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << std::endl;
    out << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << std::endl;
    out << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << std::endl;
    out << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << std::endl;
    out << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
    out << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
    out << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << std::endl;
    out << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << std::endl;
    out << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << std::endl;
    out << " - maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup << std::endl;
    out << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
    out << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
    out << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
    out << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension << std::endl;
  }
  fmt::print("{}", out.str());
}

}  // namespace wgpu_utils
