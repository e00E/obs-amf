#include "amf.h"

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
  if (init_(version(), &factory) != AMF_OK) {
    throw std::runtime_error("init");
  }
  return *factory;
}
