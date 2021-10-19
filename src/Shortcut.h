/*
 * Copyright (C) 2020, 2021 The GMetronome Team
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

#ifndef __SHORTCUT_H__
#define __SHORTCUT_H__

#include "Settings.h"
#include "Action.h"

// map a shortcut settings key to a title 
struct ShortcutEntry
{
  Glib::ustring key;
  Glib::ustring title;
};

// The returned vector contains a list of grouped ShortcutEntry,
// in which group titles have an empty key.
const std::vector<ShortcutEntry>& ShortcutList();

struct ShortcutAction
{
  Glib::ustring action_name;
  Glib::VariantBase target_value;
};

// map settings keys to action/target
const std::map<Glib::ustring, ShortcutAction> kDefaultShortcutActionMap
{
  { kKeyShortcutsQuit,
    { kActionQuit, {} }
  },
  { kKeyShortcutsShowPrimaryMenu,
    { kActionShowPrimaryMenu, {} }
  },
  { kKeyShortcutsShowProfiles,
    { kActionShowProfiles, {} }
  },
  { kKeyShortcutsShowPreferences,
    { kActionShowPreferences, {} }
  },
  { kKeyShortcutsShowShortcuts,
    { kActionShowShortcuts, {} }
  },
  { kKeyShortcutsShowHelp,
    { kActionShowHelp, {} }
  },
  { kKeyShortcutsShowAbout,
    { kActionShowAbout, {} }
  },
  { kKeyShortcutsShowMeter,
    { kActionShowMeter, {} }
  },
  { kKeyShortcutsShowTrainer,
    { kActionShowTrainer, {} }
  },
  { kKeyShortcutsStart,
    { kActionStart, {} }
  },
  { kKeyShortcutsVolumeIncrease1,
    { kActionVolumeIncrease, Glib::Variant<double>::create(1.) }
  },
  { kKeyShortcutsVolumeDecrease1,
    { kActionVolumeDecrease, Glib::Variant<double>::create(1.) }
  },
  { kKeyShortcutsVolumeIncrease10,
    { kActionVolumeIncrease, Glib::Variant<double>::create(10.) }
  },
  { kKeyShortcutsVolumeDecrease10,
    { kActionVolumeDecrease, Glib::Variant<double>::create(10.) }
  },
  { kKeyShortcutsTempoIncrease1,
    { kActionTempoIncrease, Glib::Variant<double>::create(1.) }
  },
  { kKeyShortcutsTempoDecrease1,
    { kActionTempoDecrease, Glib::Variant<double>::create(1.) }
  },
  { kKeyShortcutsTempoIncrease10,
    { kActionTempoIncrease, Glib::Variant<double>::create(10.) }
  },
  { kKeyShortcutsTempoDecrease10,
    { kActionTempoDecrease, Glib::Variant<double>::create(10.) }
  },
  { kKeyShortcutsTrainerEnabled,
    { kActionTrainerEnabled, {} }
  },
  { kKeyShortcutsMeterEnabled,
    { kActionMeterEnabled, {} }
  },
  { kKeyShortcutsMeterSelect1Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter1Simple) }
  },
  { kKeyShortcutsMeterSelect2Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter2Simple) }
  },
  { kKeyShortcutsMeterSelect3Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter3Simple) }
  },
  { kKeyShortcutsMeterSelect4Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter4Simple) }
  },
  { kKeyShortcutsMeterSelect1Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter1Compound) }
  },
  { kKeyShortcutsMeterSelect2Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter2Compound) }
  },
  { kKeyShortcutsMeterSelect3Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter3Compound) }
  },
  { kKeyShortcutsMeterSelect4Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter4Compound) }
  },
  { kKeyShortcutsMeterSelectCustom,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCustom) }
  }
};

#endif//__SHORTCUT_H__
