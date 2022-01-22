/*
 * Copyright (C) 2020 The GMetronome Team
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

#include <string>
#include "Meter.h"

struct Profile
{
  using Identifier = std::string;

  // number of UTF-8 encoded unicode characters
  static constexpr std::size_t kTitleMaxLength = 255;

  // number of UTF-8 encoded unicode characters
  static constexpr std::size_t kDescriptionMaxLength = 1024;

  static inline const std::string kDefaultTitle       = u8"Unnamed Profile";
  static inline const std::string kDefaultDescription = u8"";

  static constexpr double  kMinimumTempo           = 30.0;
  static constexpr double  kMaximumTempo           = 250.0;
  static constexpr double  kDefaultTempo           = 120.0;
  static constexpr double  kMinimumTrainerStart    = kMinimumTempo;
  static constexpr double  kMaximumTrainerStart    = kMaximumTempo;
  static constexpr double  kDefaultTrainerStart    = 80.0;
  static constexpr double  kMinimumTrainerTarget   = kMinimumTempo;
  static constexpr double  kMaximumTrainerTarget   = kMaximumTempo;
  static constexpr double  kDefaultTrainerTarget   = 160.0;
  static constexpr double  kMinimumTrainerAccel    = 1.0;
  static constexpr double  kMaximumTrainerAccel    = 1000.0;
  static constexpr double  kDefaultTrainerAccel    = 10.0;

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
    std::string   meter_select     = "meter-4-simple";
    Meter         meter_1_simple   = kMeter_1_Simple;
    Meter         meter_2_simple   = kMeter_2_Simple;
    Meter         meter_3_simple   = kMeter_3_Simple;
    Meter         meter_4_simple   = kMeter_4_Simple;
    Meter         meter_1_compound = kMeter_1_Compound;
    Meter         meter_2_compound = kMeter_2_Compound;
    Meter         meter_3_compound = kMeter_3_Compound;
    Meter         meter_4_compound = kMeter_4_Compound;
    Meter         meter_custom     = kMeter_4_Simple;

    bool          trainer_enabled   = false;
    double        trainer_start     = kDefaultTrainerStart;
    double        trainer_target    = kDefaultTrainerTarget;
    double        trainer_accel     = kDefaultTrainerAccel;
  };

  Header  header;
  Content content;
};

inline const Profile  kDefaultProfile;

#endif//GMetronome_Profile_h
