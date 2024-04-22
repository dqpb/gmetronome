/*
 * Copyright (C) 2024 The GMetronome Team
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

#ifndef GMetronome_MeterVariant_h
#define GMetronome_MeterVariant_h

#include "Meter.h"

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

#endif//GMetronome_MeterVariant_h
