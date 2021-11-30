#pragma once

#include "gsl.h"
#include "module.h"

#include <AMF/core/Factory.h>

// C++ wrapper for AMF
class Amf {
  Module amf_dll;
  not_null<AMFQueryVersion_Fn> query_version;
  not_null<AMFInit_Fn> init_;

public:
  Amf();
  amf_uint64 version() const;
  // Result's lifetime is tied to this instance's lifetime.
  amf::AMFFactory &init() const;
};
