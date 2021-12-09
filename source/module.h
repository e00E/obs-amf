#pragma once

#include "gsl.h"
#include "windows.h"

// must come after windows.h
#include <libloaderapi.h>

#include <memory>

// C++ wrapper for LoadLibrary
class Module {
  struct Free {
    void operator()(HMODULE hmodule) const noexcept;
  };
  std::unique_ptr<std::remove_pointer<HMODULE>::type, Free> inner;

public:
  Module(not_null<czstring> name);
  HMODULE get() const;
};

template <typename T>
concept FunctionPointer = std::is_pointer<T>::value &&
    std::is_function<typename std::remove_pointer<T>::type>::value;

// C++ wrapper for GetProcAddress
template <FunctionPointer T>
not_null<T> get_proc_address(HMODULE module, not_null<czstring> name) {
  const auto address = GetProcAddress(module, name);
  if (!address) {
    throw std::runtime_error("GetProcAddress");
  }
  return reinterpret_cast<T>(address);
}
