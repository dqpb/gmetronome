/*
 * Copyright (C) 2020-2022 The GMetronome Team
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

#ifndef GMetronome_Profile_h
#define GMetronome_Profile_h

#include "Meter.h"
#include <string>
#include <glibmm/i18n.h>

struct Profile
{
  using Identifier = std::string;

  enum class TrainerMode
  {
    kContinuous = 1,
    kStepwise = 2
  };

  // Number of UTF-8 encoded unicode characters
  static constexpr std::size_t kTitleMaxLength = 255;

  // Number of UTF-8 encoded unicode characters
  static constexpr std::size_t kDescriptionMaxLength = 1024;

  // Default title for new profiles
  static inline const std::string kDefaultTitle            = C_("Profile", "New Profile");
  // Placeholder title for untitled profiles
  static inline const std::string kDefaultTitlePlaceholder = C_("Profile", "Untitled Profile");
  // Title of duplicated profiles, %1 will be replaced by the old title
  static inline const std::string kDefaultTitleDuplicate   = C_("Profile", "%1 (copy)");
  // Default description for new profiles
  static inline const std::string kDefaultDescription      = "";

  static constexpr double      kMinTempo               = 30.0;
  static constexpr double      kMaxTempo               = 250.0;
  static constexpr double      kDefaultTempo           = 120.0;
  static constexpr TrainerMode kDefaultTrainerMode     = TrainerMode::kContinuous;
  static constexpr double      kMinTrainerTarget       = kMinTempo;
  static constexpr double      kMaxTrainerTarget       = kMaxTempo;
  static constexpr double      kDefaultTrainerTarget   = 160.0;
  static constexpr double      kMinTrainerAccel        = 1.0;
  static constexpr double      kMaxTrainerAccel        = 1000.0;
  static constexpr double      kDefaultTrainerAccel    = 10.0;
  static constexpr double      kMinTrainerStep         = 1.0;
  static constexpr double      kMaxTrainerStep         = 99.0;
  static constexpr double      kDefaultTrainerStep     = 2.0;
  static constexpr int         kMinTrainerHold         = 1;
  static constexpr int         kMaxTrainerHold         = 99;
  static constexpr int         kDefaultTrainerHold     = 8;

  struct Header
  {
    std::string  title        = kDefaultTitle;
    std::string  description  = kDefaultDescription;
  };

  struct Primer
  {
    Profile::Identifier id;
    Profile::Header     header;
  };

  struct Content
  {
    double        tempo             = kDefaultTempo;

    bool          meter_enabled    = false;
    std::string   meter_select     = "meter-simple-4";
    Meter         meter_simple_2   = kMeterSimple2;
    Meter         meter_simple_3   = kMeterSimple3;
    Meter         meter_simple_4   = kMeterSimple4;
    Meter         meter_compound_2 = kMeterCompound2;
    Meter         meter_compound_3 = kMeterCompound3;
    Meter         meter_compound_4 = kMeterCompound4;
    Meter         meter_custom     = kMeterSimple4;

    bool          trainer_enabled   = false;
    TrainerMode   trainer_mode      = kDefaultTrainerMode;
    double        trainer_target    = kDefaultTrainerTarget;
    double        trainer_accel     = kDefaultTrainerAccel;
    double        trainer_step      = kDefaultTrainerStep;
    int           trainer_hold      = kDefaultTrainerHold;

    std::string   sound_theme_id    = "";
  };

  Header  header;
  Content content;
};

inline const Profile  kDefaultProfile;

#endif//GMetronome_Profile_h
