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

#include "Shortcut.h"
#include <glibmm/i18n.h>

const std::vector<ShortcutEntry>& ShortcutList()
{
  static const std::vector<ShortcutEntry> kShortcutList
    {
      { {}, C_("Shortcut group title", "Application") },
      { settings::kKeyShortcutsShowPrimaryMenu,       C_("Shortcut title", "Show Primary Menu") },
      { settings::kKeyShortcutsShowProfiles,          C_("Shortcut title", "Show Profiles") },
      { settings::kKeyShortcutsShowPreferences,       C_("Shortcut title", "Show Preferences") },
      { settings::kKeyShortcutsShowShortcuts,         C_("Shortcut title", "Show Keyboard Shortcuts") },
      { settings::kKeyShortcutsShowHelp,              C_("Shortcut title", "Show Help") },
      { settings::kKeyShortcutsShowAbout,             C_("Shortcut title", "Show About") },
      { settings::kKeyShortcutsQuit,                  C_("Shortcut title", "Quit") },

      { {}, C_("Shortcut group title", "View") },
      { settings::kKeyShortcutsShowPendulum,          C_("Shortcut title", "Show Pendulum") },
      { settings::kKeyShortcutsFullScreen,            C_("Shortcut title", "Full Screen") },

      { {}, C_("Shortcut group title", "Transport") },
      { settings::kKeyShortcutsStart,                  C_("Shortcut title", "Start / Stop") },

      { {}, C_("Shortcut group title", "Volume") },
      { settings::kKeyShortcutsVolumeIncrease1,         C_("Shortcut title", "Volume +1") },
      { settings::kKeyShortcutsVolumeDecrease1,         C_("Shortcut title", "Volume -1") },
      { settings::kKeyShortcutsVolumeIncrease10,        C_("Shortcut title", "Volume +10") },
      { settings::kKeyShortcutsVolumeDecrease10,        C_("Shortcut title", "Volume -10") },

      { {}, C_("Shortcut group title", "Tempo") },
      { settings::kKeyShortcutsTempoIncrease1,         C_("Shortcut title", "Tempo +1") },
      { settings::kKeyShortcutsTempoDecrease1,         C_("Shortcut title", "Tempo -1") },
      { settings::kKeyShortcutsTempoIncrease10,        C_("Shortcut title", "Tempo +10") },
      { settings::kKeyShortcutsTempoDecrease10,        C_("Shortcut title", "Tempo -10") },
      { settings::kKeyShortcutsTempoTap,               C_("Shortcut title", "Tempo Tap") },

      { {}, C_("Shortcut group title", "Accents") },
      { settings::kKeyShortcutsMeterEnabled,           C_("Shortcut title", "Enable / Disable Accentuation") },
      { settings::kKeyShortcutsMeterSelect1Simple,     C_("Shortcut title", "Select Simple 1 Meter") },
      { settings::kKeyShortcutsMeterSelect2Simple,     C_("Shortcut title", "Select Simple 2 Meter") },
      { settings::kKeyShortcutsMeterSelect3Simple,     C_("Shortcut title", "Select Simple 3 Meter") },
      { settings::kKeyShortcutsMeterSelect4Simple,     C_("Shortcut title", "Select Simple 4 Meter") },
      { settings::kKeyShortcutsMeterSelect1Compound,   C_("Shortcut title", "Select Compound 1 Meter") },
      { settings::kKeyShortcutsMeterSelect2Compound,   C_("Shortcut title", "Select Compound 2 Meter") },
      { settings::kKeyShortcutsMeterSelect3Compound,   C_("Shortcut title", "Select Compound 3 Meter") },
      { settings::kKeyShortcutsMeterSelect4Compound,   C_("Shortcut title", "Select Compound 4 Meter") },
      { settings::kKeyShortcutsMeterSelectCustom,      C_("Shortcut title", "Select Custom Meter") },

      { {}, C_("Shortcut group title", "Trainer") },
      { settings::kKeyShortcutsTrainerEnabled,         C_("Shortcut title", "Enable / Disable Trainer") }
    };

  return kShortcutList;
}

const std::map<Glib::ustring, ShortcutAction> kDefaultShortcutActionMap
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
