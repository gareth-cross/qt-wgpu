#include "wgpu_error_scope.hpp"

#include "wgpu_fmt.hpp"

namespace wgpu_utils {

wgpu_error_scope::wgpu_error_scope(const wgpu::Device& device, const std::string_view name)
    : device_(device), name_(name) {
  device_.PushErrorScope(wgpu::ErrorFilter::Validation);
}
wgpu_error_scope::~wgpu_error_scope() {
  device_.PopErrorScope(wgpu::CallbackMode::AllowSpontaneous,
                        [&](wgpu::PopErrorScopeStatus, wgpu::ErrorType type, wgpu::StringView message) {
                          if (type != wgpu::ErrorType::NoError) {
                            fmt::print("Error [scope = {}, type = {}]: {}\n", name_, fmt_enum(type), message);
                          }
                        });
}

}  // namespace wgpu_utils
