#include "module.h"

#include <stdexcept>

void Module::Free::operator()(HMODULE hmodule) const noexcept {
  FreeLibrary(hmodule);
}

Module::Module(not_null<czstring> name) : inner{LoadLibraryA(name)} {
  if (!inner) {
    throw std::runtime_error("LoadLibraryW");
  }
}

HMODULE Module::get() const { return inner.get(); }
