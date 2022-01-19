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

#ifndef GMetronome_Shortcut_h
#define GMetronome_Shortcut_h

#include "Settings.h"
#include "Action.h"

// map a shortcut settings key to a title
struct ShortcutEntry
{
  Glib::ustring key;
  Glib::ustring title;
};

// Returns a list of grouped shortcut entries as they appear in the shortcuts
// tree view in the settings dialog. The group titles have an empty key.
const std::vector<ShortcutEntry>& ShortcutList();

struct ShortcutAction
{
  Glib::ustring action_name;
  Glib::VariantBase target_value;
};

// map settings keys to action/target
inline const std::map<Glib::ustring, ShortcutAction> kDefaultShortcutActionMap
{
  { settings::kKeyShortcutsQuit,
    { kActionQuit, {} }
  },
  { settings::kKeyShortcutsShowPrimaryMenu,
    { kActionShowPrimaryMenu, {} }
  },
  { settings::kKeyShortcutsShowProfiles,
    { kActionShowProfiles, {} }
  },
  { settings::kKeyShortcutsShowPreferences,
    { kActionShowPreferences, {} }
  },
  { settings::kKeyShortcutsShowShortcuts,
    { kActionShowShortcuts, {} }
  },
  { settings::kKeyShortcutsShowHelp,
    { kActionShowHelp, {} }
  },
  { settings::kKeyShortcutsShowAbout,
    { kActionShowAbout, {} }
  },
  { settings::kKeyShortcutsShowPendulum,
    { kActionShowPendulum, {} }
  },
  { settings::kKeyShortcutsShowMeter,
    { kActionShowMeter, {} }
  },
  { settings::kKeyShortcutsShowTrainer,
    { kActionShowTrainer, {} }
  },
  { settings::kKeyShortcutsFullScreen,
    { kActionFullScreen, {} }
  },
  { settings::kKeyShortcutsStart,
    { kActionStart, {} }
  },
  { settings::kKeyShortcutsVolumeIncrease1,
    { kActionVolumeIncrease, Glib::Variant<double>::create(1.) }
  },
  { settings::kKeyShortcutsVolumeDecrease1,
    { kActionVolumeDecrease, Glib::Variant<double>::create(1.) }
  },
  { settings::kKeyShortcutsVolumeIncrease10,
    { kActionVolumeIncrease, Glib::Variant<double>::create(10.) }
  },
  { settings::kKeyShortcutsVolumeDecrease10,
    { kActionVolumeDecrease, Glib::Variant<double>::create(10.) }
  },
  { settings::kKeyShortcutsTempoIncrease1,
    { kActionTempoIncrease, Glib::Variant<double>::create(1.) }
  },
  { settings::kKeyShortcutsTempoDecrease1,
    { kActionTempoDecrease, Glib::Variant<double>::create(1.) }
  },
  { settings::kKeyShortcutsTempoIncrease10,
    { kActionTempoIncrease, Glib::Variant<double>::create(10.) }
  },
  { settings::kKeyShortcutsTempoDecrease10,
    { kActionTempoDecrease, Glib::Variant<double>::create(10.) }
  },
  { settings::kKeyShortcutsTempoTap,
    { kActionTempoTap, {} }
  },
  { settings::kKeyShortcutsTrainerEnabled,
    { kActionTrainerEnabled, {} }
  },
  { settings::kKeyShortcutsMeterEnabled,
    { kActionMeterEnabled, {} }
  },
  { settings::kKeyShortcutsMeterSelect1Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter1Simple) }
  },
  { settings::kKeyShortcutsMeterSelect2Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter2Simple) }
  },
  { settings::kKeyShortcutsMeterSelect3Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter3Simple) }
  },
  { settings::kKeyShortcutsMeterSelect4Simple,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter4Simple) }
  },
  { settings::kKeyShortcutsMeterSelect1Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter1Compound) }
  },
  { settings::kKeyShortcutsMeterSelect2Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter2Compound) }
  },
  { settings::kKeyShortcutsMeterSelect3Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter3Compound) }
  },
  { settings::kKeyShortcutsMeterSelect4Compound,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeter4Compound) }
  },
  { settings::kKeyShortcutsMeterSelectCustom,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCustom) }
  }
};

#endif//GMetronome_Shortcut_h
