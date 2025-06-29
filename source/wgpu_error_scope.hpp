#pragma once
#include <webgpu/webgpu_cpp.h>

#define WGPU_ERROR_SCOPE_CONCAT_(a, b) a##b
#define WGPU_ERROR_SCOPE_CONCAT(a, b) WGPU_ERROR_SCOPE_CONCAT_(a, b)
#define WGPU_ERROR_SCOPE(device, name) \
  wgpu_utils::wgpu_error_scope WGPU_ERROR_SCOPE_CONCAT(__scope, __LINE__) { device, name }

#define WGPU_ERROR_FUNCTION_SCOPE(device) WGPU_ERROR_SCOPE(device, __FUNCTION__)

namespace wgpu_utils {

// Log errors within a specific scope. On destruction, print errors to stdout.
class wgpu_error_scope {
 public:
  wgpu_error_scope(const wgpu::Device& device, const std::string_view name);
  ~wgpu_error_scope();

 private:
  const wgpu::Device& device_;
  std::string_view name_;
};

}  // namespace wgpu_utils
