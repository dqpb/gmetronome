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
#include <bitset>

enum Accent
{
  kAccentOff    = 0,
  kAccentWeak   = 1,
  kAccentMid    = 2,
  kAccentStrong = 3
};

constexpr int kNumAccents = 4;

using AccentFlags = std::bitset<kNumAccents>;

constexpr AccentFlags kAccentMaskOff    {0b0001};
constexpr AccentFlags kAccentMaskWeak   {0b0010};
constexpr AccentFlags kAccentMaskMid    {0b0100};
constexpr AccentFlags kAccentMaskStrong {0b1000};
constexpr AccentFlags kAccentMaskAll    {0b1111};

using AccentPattern = std::vector<Accent>;

// TODO: make constexpr when switching to c++20
inline const AccentPattern kAccentPattern1 = {
  kAccentMid
};
inline const AccentPattern kAccentPattern2 = {
  kAccentStrong,
  kAccentMid
};
inline const AccentPattern kAccentPattern3 = {
  kAccentStrong,
  kAccentMid,
  kAccentMid
};
inline const AccentPattern kAccentPattern4 = {
  kAccentStrong,
  kAccentMid,
  kAccentMid,
  kAccentMid
};
inline const AccentPattern kAccentPatternSimple1 = {
  kAccentMid,
  kAccentOff
};
inline const AccentPattern kAccentPatternSimple2 = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
inline const AccentPattern kAccentPatternSimple3 = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
inline const AccentPattern kAccentPatternSimple4 = {
  kAccentStrong,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentMid,
  kAccentOff
};
inline const AccentPattern kAccentPatternCompound1 = {
  kAccentMid,
  kAccentOff,
  kAccentOff
};
inline const AccentPattern kAccentPatternCompound2 = {
  kAccentStrong,
  kAccentOff,
  kAccentOff,
  kAccentMid,
  kAccentOff,
  kAccentOff
};
inline const AccentPattern kAccentPatternCompound3 = {
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
inline const AccentPattern kAccentPatternCompound4 = {
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

  static inline constexpr int kMaxBeats     = 12;
  static inline constexpr int kMaxDivision  = 4;

  explicit Meter(int division = kSimpleMeter,
                 int beats = kQuadrupleMeter,
                 const AccentPattern& accents = kAccentPatternSimple4);

  int division() const
    { return division_; }

  void setDivision(int division);

  int beats() const
    { return beats_; }

  void setBeats(int beats);

  const AccentPattern& accents() const
    { return accents_; }

  void setAccentPattern(AccentPattern accents);

  void setAccent(std::size_t index, Accent accent);

private:
  int division_;
  int beats_;
  AccentPattern accents_;

  void checkDataIntegrity();
};

inline const Meter kMeter1 {kNoDivision, kSingleMeter, kAccentPattern1};
inline const Meter kMeter2 {kNoDivision, kDupleMeter, kAccentPattern2};
inline const Meter kMeter3 {kNoDivision, kTripleMeter, kAccentPattern3};
inline const Meter kMeter4 {kNoDivision, kQuadrupleMeter, kAccentPattern4};

inline const Meter kMeterSimple1 {kSimpleMeter, kSingleMeter, kAccentPatternSimple1};
inline const Meter kMeterSimple2 {kSimpleMeter, kDupleMeter, kAccentPatternSimple2};
inline const Meter kMeterSimple3 {kSimpleMeter, kTripleMeter, kAccentPatternSimple3};
inline const Meter kMeterSimple4 {kSimpleMeter, kQuadrupleMeter, kAccentPatternSimple4};

inline const Meter kMeterCompound1 {kCompoundMeter, kSingleMeter, kAccentPatternCompound1};
inline const Meter kMeterCompound2 {kCompoundMeter, kDupleMeter, kAccentPatternCompound2};
inline const Meter kMeterCompound3 {kCompoundMeter, kTripleMeter, kAccentPatternCompound3};
inline const Meter kMeterCompound4 {kCompoundMeter, kQuadrupleMeter, kAccentPatternCompound4};


#include <glibmm/variant.h>

// Specialization of Glib::Variant<> class for Accent enum type.
template <>
class GLIBMM_API Glib::Variant<Accent> : public Glib::VariantBase
{
public:
  using CType = guint8;

  // Default constructor.
  Variant() : VariantBase() {}

  explicit Variant(GVariant* castitem, bool take_a_reference = false)
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
  Variant() : VariantContainerBase() {}

  explicit Variant(GVariant* castitem, bool take_a_reference = false)
    : VariantContainerBase(castitem, take_a_reference) {}

  static Variant<Meter> create(const Meter& data) {
    std::vector<Glib::VariantBase> variants =
      {
        Variant<int>::create(data.division()),
        Variant<int>::create(data.beats()),
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
    Meter data {
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
