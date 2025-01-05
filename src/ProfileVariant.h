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

#ifndef GMetronome_ProfileVariant_h
#define GMetronome_ProfileVariant_h

#include "Profile.h"
#include <glibmm/variant.h>

// Specialization of Glib::Variant<> class for Profile::TrainerMode enum type.
template <>
class GLIBMM_API Glib::Variant<Profile::TrainerMode> : public Glib::VariantBase
{
public:
  using CType = gint32;

  // Default constructor.
  Variant() : VariantBase() {}

  explicit Variant(GVariant* castitem, bool take_a_reference = false)
  : VariantBase(castitem, take_a_reference) {}

  static const VariantType& variant_type() G_GNUC_CONST {
    static VariantType type(G_VARIANT_TYPE_INT32);
    return type;
  }

  static Variant<Profile::TrainerMode> create(Profile::TrainerMode data) {
    auto result = Variant<Profile::TrainerMode>(g_variant_new_int32(static_cast<CType>(data)));
    return result;
  }

  Profile::TrainerMode get() const {
    return static_cast<Profile::TrainerMode>(g_variant_get_int32(gobject_));
  }
};

#endif//GMetronome_ProfileVariant_h
