#pragma once
#include <fmt/core.h>
#include <webgpu/webgpu_cpp.h>
#include <magic_enum/magic_enum.hpp>

namespace wgpu_utils {

// Format an enum using magic enum.
template <typename T>
struct fmt_enum {
  static_assert(std::is_enum_v<T>);

  explicit constexpr fmt_enum(T value) noexcept : value(value) {}

  T value;
};

}  // namespace wgpu_utils

// Formatter for wgpu::StringView
template <>
struct fmt::formatter<wgpu::StringView> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

  template <typename FormatContext>
  constexpr auto format(const wgpu::StringView& message, FormatContext& ctx) const -> decltype(ctx.out()) {
    if (message.data) {
      return fmt::format_to(ctx.out(), "{}", std::string_view(message));
    } else {
      return fmt::format_to(ctx.out(), "<empty string>");
    }
  }
};

template <typename T>
struct fmt::formatter<wgpu_utils::fmt_enum<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

  template <typename FormatContext>
  constexpr auto format(const wgpu_utils::fmt_enum<T>& e, FormatContext& ctx) const -> decltype(ctx.out()) {
    const auto name = magic_enum::enum_name(e.value);
    if (name.data()) {
      return fmt::format_to(ctx.out(), "{}", name);
    } else {
      return fmt::format_to(ctx.out(), "{:#x}", static_cast<std::underlying_type_t<T>>(e.value));
    }
  }
};
