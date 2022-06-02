#include "settings.h"

#include "gsl.h"
#include "util.h"

BoolSetting::BoolSetting(not_null<czstring> name,
                         not_null<czstring> description,
                         not_null<cwzstring> amf_name, bool default_) noexcept
    : name{name}, description{description}, amf_name{amf_name}, default_{
                                                                    default_} {}

void BoolSetting::obs_property(obs_properties &properties) const noexcept {
  ASSERT_(obs_properties_add_bool(&properties, name, description));
}

void BoolSetting::obs_default(obs_data &data) const noexcept {
  obs_data_set_default_bool(&data, name, default_);
}

void BoolSetting::amf_property(obs_data &data,
                               amf::AMFComponent &encoder) const {
  const auto value{obs_data_get_bool(&data, name)};
  set_property_fallible(encoder, amf_name, value);
}

IntSetting::IntSetting(not_null<czstring> name, not_null<czstring> description,
                       not_null<cwzstring> amf_name, int min, int max,
                       int default_) noexcept
    : name{name}, description{description}, amf_name{amf_name}, min{min},
      max{max}, default_{default_} {
  ASSERT_(this->default_ >= this->min && this->default_ <= this->max);
  ASSERT_(this->min <= this->max);
}

void IntSetting::obs_property(obs_properties &properties) const noexcept {
  if (max - min <= 10000) {
    ASSERT_(obs_properties_add_int_slider(&properties, name, description, min,
                                          max, 1));
  } else {
    ASSERT_(
        obs_properties_add_int(&properties, name, description, min, max, 1));
  }
}

void IntSetting::obs_default(obs_data &data) const noexcept {
  obs_data_set_default_int(&data, name, default_);
}

void IntSetting::amf_property(obs_data &data,
                              amf::AMFComponent &encoder) const {
  const auto value{gsl::narrow<int>(obs_data_get_int(&data, name))};
  set_property_fallible(encoder, amf_name, static_cast<int64_t>(value));
}

EnumSetting::EnumSetting(
    not_null<czstring> name, not_null<czstring> description,
    not_null<cwzstring> amf_name,
    std::vector<std::tuple<int, not_null<czstring>>> &&values,
    size_t default_) noexcept
    : name{name}, description{description}, amf_name{amf_name}, values{values},
      default_{default_} {
  ASSERT_(this->default_ < this->values.size());
}

void EnumSetting::obs_property(obs_properties &properties) const noexcept {
  const auto p =
      obs_properties_add_list(&properties, name, description,
                              OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  ASSERT_(p);
  for (const auto &[value, name] : values) {
    obs_property_list_add_int(p, name, value);
  }
}

void EnumSetting::obs_default(obs_data &data) const noexcept {
  obs_data_set_default_int(&data, name, std::get<0>(values[default_]));
}

void EnumSetting::amf_property(obs_data &data,
                               amf::AMFComponent &encoder) const {
  const auto value{gsl::narrow<int>(obs_data_get_int(&data, name))};
  set_property_fallible(encoder, amf_name, static_cast<int64_t>(value));
}
