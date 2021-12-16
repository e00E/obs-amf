#include "amf.h"

#include "util.h"

#include <AMF/core/Version.h>

namespace {
struct Version {
  uint64_t original;
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
  uint16_t build;

  Version(uint64_t v)
      : major{AMF_GET_MAJOR_VERSION(v)}, minor{AMF_GET_MINOR_VERSION(v)},
        patch{AMF_GET_SUBMINOR_VERSION(v)}, build{AMF_GET_BUILD_VERSION(v)} {}
};
} // namespace

Amf::Amf()
    : amf_dll{AMF_DLL_NAMEA},
      query_version{get_proc_address<AMFQueryVersion_Fn>(
          amf_dll.get(), AMF_QUERY_VERSION_FUNCTION_NAME)},
      init_{get_proc_address<AMFInit_Fn>(amf_dll.get(),
                                         AMF_INIT_FUNCTION_NAME)} {}

amf_uint64 Amf::version() const {
  amf_uint64 version{0};
  if (query_version(&version) != AMF_OK) {
    throw std::runtime_error("query_version");
  }
  return version;
}

amf::AMFFactory &Amf::init() const {
  amf::AMFFactory *factory{nullptr};
  const Version runtime{version()};
  const Version compiletime{AMF_FULL_VERSION};
  log(LOG_INFO, "AMF runtime version    : {}.{}.{} build {} raw {}",
      runtime.major, runtime.minor, runtime.patch, runtime.build,
      runtime.original);
  log(LOG_INFO, "AMF compiletime version: {}.{}.{} build {} raw {}",
      compiletime.major, compiletime.minor, compiletime.patch,
      compiletime.build, compiletime.original);
  // Use compiletime version for compatibility.
  if (init_(compiletime.original, &factory) != AMF_OK) {
    throw std::runtime_error("init");
  }
  return *factory;
}
