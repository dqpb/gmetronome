/*
 * Copyright (C) 2021 The GMetronome Team
 *
 * This file is part of GMetronome.
 *
 * GMetronome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GMetronome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMetronome.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GMetronome_Meter_h
#define GMetronome_Meter_h

#include <vector>

enum Accent
{
  kAccentOff    = 0,
  kAccentWeak   = 1,
  kAccentMid    = 2,
  kAccentStrong = 3
};

using AccentPattern = std::vector<Accent>;

const AccentPattern kAccentPattern_1 = {
  kAccentMid
};
const AccentPattern kAccentPattern_2 = {
  kAccentStrong,
  kAccentMid
};
const AccentPattern kAccentPattern_3 = {
  kAccentStrong,
  kAccentMid,
  kAccentMid
};
const AccentPattern kAccentPattern_4 = {
  kAccentStrong,
  kAccentMid,
  kAccentMid,
  kAccentMid
};
const AccentPattern kAccentPattern_1_Simple = {
  kAccentMid,
  kAccentOff
};
const AccentPattern kAccentPattern_2_Simple = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
const AccentPattern kAccentPattern_3_Simple = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
const AccentPattern kAccentPattern_4_Simple = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
const AccentPattern kAccentPattern_1_Compound = {
  kAccentMid,
  kAccentOff,
  kAccentOff
};
const AccentPattern kAccentPattern_2_Compound = {
  kAccentStrong,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff
};
const AccentPattern kAccentPattern_3_Compound = {
  kAccentStrong,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff
};
const AccentPattern kAccentPattern_4_Compound = {
  kAccentStrong,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff
};

enum MeterBeats
{
  kSingleMeter    = 1,
  kDupleMeter     = 2,
  kTripleMeter    = 3,
  kQuadrupleMeter = 4
};

enum MeterDivision
{
  kNoDivision    = 1,
  kSimpleMeter   = 2,
  kCompoundMeter = 3
};

class Meter {
public:
  Meter(int beats = kQuadrupleMeter,
        int division = kSimpleMeter,
        const AccentPattern& accents = kAccentPattern_4_Simple);

  int beats() const
    { return beats_; }

  void setBeats(int beats);

  int division() const
    { return division_; }

  void setDivision(int division);

  const AccentPattern& accents() const
    { return accents_; }

  void setAccentPattern(const AccentPattern& accents);

  void setAccentPattern(AccentPattern&& accents);

  void setAccent(std::size_t index, Accent accent);

private:
  int beats_;
  int division_;
  AccentPattern accents_;

  void check_data_integrity();
};

const Meter kMeter_1 = {kSingleMeter, kNoDivision, kAccentPattern_1};
const Meter kMeter_2 = {kDupleMeter, kNoDivision, kAccentPattern_2};
const Meter kMeter_3 = {kTripleMeter, kNoDivision, kAccentPattern_3};
const Meter kMeter_4 = {kQuadrupleMeter, kNoDivision, kAccentPattern_4};

const Meter kMeter_1_Simple = {kSingleMeter, kSimpleMeter, kAccentPattern_1_Simple};
const Meter kMeter_2_Simple = {kDupleMeter, kSimpleMeter, kAccentPattern_2_Simple};
const Meter kMeter_3_Simple = {kTripleMeter, kSimpleMeter, kAccentPattern_3_Simple};
const Meter kMeter_4_Simple = {kQuadrupleMeter, kSimpleMeter, kAccentPattern_4_Simple};

const Meter kMeter_1_Compound = {kSingleMeter, kCompoundMeter, kAccentPattern_1_Compound};
const Meter kMeter_2_Compound = {kDupleMeter, kCompoundMeter, kAccentPattern_2_Compound};
const Meter kMeter_3_Compound = {kTripleMeter, kCompoundMeter, kAccentPattern_3_Compound};
const Meter kMeter_4_Compound = {kQuadrupleMeter, kCompoundMeter, kAccentPattern_4_Compound};


#include <glibmm/variant.h>

// Specialization of Glib::Variant<> class for Accent enum type.
template <>
class GLIBMM_API Glib::Variant<Accent> : public Glib::VariantBase
{
public:
  using CType = guint8;

  // Default constructor.
  Variant<Accent>() : VariantBase() {}

  explicit Variant<Accent>(GVariant* castitem, bool take_a_reference = false)
  : VariantBase(castitem, take_a_reference) {}

  static const VariantType& variant_type() G_GNUC_CONST {
    static VariantType type(G_VARIANT_TYPE_BYTE);
    return type;
  }

  static Variant<Accent> create(Accent data) {
    auto result = Variant<Accent>(g_variant_new_byte(data));
    return result;
  }

  Accent get() const {
    return static_cast<Accent>(g_variant_get_byte(gobject_));
  }
};


// Specialization of Glib::Variant<> class for Meter type.
template <>
class Glib::Variant<Meter> : public Glib::VariantContainerBase
{
public:
  using CppContainerType = Meter;

  // Default constructor.
  Variant<Meter>() : VariantContainerBase() {}

  explicit Variant<Meter>(GVariant* castitem, bool take_a_reference = false)
    : VariantContainerBase(castitem, take_a_reference) {}

  static Variant<Meter> create(const Meter& data) {
    std::vector<Glib::VariantBase> variants =
      {
        Variant<int>::create(data.beats()),
        Variant<int>::create(data.division()),
        Variant<AccentPattern>::create(data.accents())
      };

    GVariant* var_array[3] =
      {
        const_cast<GVariant*>(variants[0].gobj()),
        const_cast<GVariant*>(variants[1].gobj()),
        const_cast<GVariant*>(variants[2].gobj())
      };

    Variant<Meter> result = Variant<Meter>(
      g_variant_new_tuple(var_array, variants.size()));

    return result;
  }

  static const VariantType& variant_type() G_GNUC_CONST {
    static std::vector<VariantType> types =
      {
        Variant<int>::variant_type(),
        Variant<int>::variant_type(),
        Variant<AccentPattern>::variant_type()
      };

    static auto type = VariantType::create_tuple(types);

    return type;
  }

  template<class T>
  T get_child(gsize index) const {
    Variant<T> entry;
    VariantContainerBase::get_child(entry, index);
    return entry.get();
  }

  template<class T>
  Variant<T> get_child_variant(gsize index) const {
    Variant<T> entry;
    VariantContainerBase::get_child(entry, index);
    return entry;
  }

  Meter get() const {
    Meter data = {
      get_child<int>(0),
      get_child<int>(1),
      get_child<AccentPattern>(2)
    };
    return data;
  }

  VariantIter get_iter() const {
    const auto type = variant_type();
    return VariantContainerBase::get_iter(type);
  }
};

#endif//GMetronome_Meter_h
