#pragma once

#include "gsl.h"

#include <obs-module.h>

#include <AMF/core/Platform.h>
#include <AMF/core/PropertyStorage.h>
#include <fmt/format.h>

#include <concepts>
#include <stdexcept>
#include <string_view>

// Better than dealing with printf style formatting even if less efficient
// because of the intermediate string we allocate.
template <typename... T>
inline void log(int log_level, fmt::format_string<T...> fmt, T &&...args) {
  blog(log_level, "amftest: %s", fmt::format(fmt, args...).c_str());
}

std::string wstring_to_string(not_null<cwzstring> wstring);

// Assert that always runs. On false logs location and terminates.
#define ASSERT_(condition)                                                     \
  if (!(condition)) {                                                          \
    log(LOG_ERROR, "assertion failure in file {} line {}", __FILE__,           \
        __LINE__);                                                             \
    std::terminate();                                                          \
  }

template <typename T>
concept AmfVariant = std::constructible_from<amf::AMFVariant, T>;

// set property or throw
template <AmfVariant T>
void set_property(amf::AMFPropertyStorage &storage, not_null<cwzstring> name,
                  const T &value) {
  if (storage.SetProperty(name, value) != AMF_OK) {
    throw std::runtime_error(
        fmt::format("SetProperty {} {}", wstring_to_string(name), value));
  }
}

// like above but with logging and without throwing
template <AmfVariant T>
void set_property_fallible(amf::AMFPropertyStorage &storage,
                           not_null<cwzstring> name, const T &value) noexcept {
  if (storage.SetProperty(name, value) == AMF_OK) {
    log(LOG_INFO,
        fmt::format("SetProperty OK {} {}", wstring_to_string(name), value));
  } else {
    log(LOG_ERROR,
        fmt::format("SetProperty ERR {} {}", wstring_to_string(name), value));
  }
}

// get property or throw
template <AmfVariant T>
T get_property(amf::AMFPropertyStorage &storage, not_null<cwzstring> name) {
  T value;
  if (storage.GetProperty(name, &value) != AMF_OK) {
    throw std::runtime_error(
        fmt::format("GetProperty {} {}", wstring_to_string(name)));
  }
  return value;
}

// fmt integration for custom types

template <> struct fmt::formatter<AMFRate> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const AMFRate &v, FormatContext &ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "({} / {})", v.num, v.den);
  }
};
