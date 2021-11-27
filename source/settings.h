#pragma once

// In OBS plugin configuration is shown to users through obs_properties and
// persisted through obs_data. In AMF configuration happens through SetProperty
// and hardcoded key values. This header connects them together to minimize
// boilerplate.

#include "gsl.h"

#include <AMF/components/Component.h>
#include <obs-module.h>

#include <tuple>
#include <vector>

// A generic configuration value.
struct Setting {
  virtual ~Setting() noexcept = default;
  // Create the graphical user facing setting.
  virtual void obs_property(obs_properties &) const noexcept = 0;
  // Set the default value for the setting.
  virtual void obs_default(obs_data &) const noexcept = 0;
  // Set the setting on an AMF componenet (the encoder).
  virtual void amf_property(obs_data &, amf::AMFComponent &) const = 0;
};

class BoolSetting : public Setting {
  not_null<czstring> name;
  not_null<czstring> description;
  not_null<cwzstring> amf_name;
  bool default_;

public:
  BoolSetting(not_null<czstring> name, not_null<czstring> description,
              not_null<cwzstring> amf_name, bool default_) noexcept;
  void obs_property(obs_properties &properties) const noexcept override;
  void obs_default(obs_data &data) const noexcept override;
  void amf_property(obs_data &data, amf::AMFComponent &encoder) const override;
};

class IntSetting : public Setting {
  not_null<czstring> name;
  not_null<czstring> description;
  not_null<cwzstring> amf_name;
  int min;
  int max;
  int default_;

public:
  IntSetting(not_null<czstring> name, not_null<czstring> description,
             not_null<cwzstring> amf_name, int min, int max,
             int default_) noexcept;
  void obs_property(obs_properties &properties) const noexcept override;
  void obs_default(obs_data &data) const noexcept override;
  void amf_property(obs_data &data, amf::AMFComponent &encoder) const override;
};

class EnumSetting : public Setting {
  not_null<czstring> name;
  not_null<czstring> description;
  not_null<cwzstring> amf_name;
  // value, name
  std::vector<std::tuple<int, not_null<czstring>>> values;
  // index into values
  size_t default_;

public:
  EnumSetting(not_null<czstring> name, not_null<czstring> description,
              not_null<cwzstring> amf_name,
              std::vector<std::tuple<int, not_null<czstring>>> &&values,
              size_t default_) noexcept;
  void obs_property(obs_properties &properties) const noexcept override;
  void obs_default(obs_data &data) const noexcept override;
  void amf_property(obs_data &data, amf::AMFComponent &encoder) const override;
};
