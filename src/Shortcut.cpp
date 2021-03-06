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
      { settings::kKeyShortcutsVolumeIncrease1,         C_("Shortcut title", "Volume +1 Percent") },
      { settings::kKeyShortcutsVolumeDecrease1,         C_("Shortcut title", "Volume -1 Percent") },
      { settings::kKeyShortcutsVolumeIncrease10,        C_("Shortcut title", "Volume +10 Percent") },
      { settings::kKeyShortcutsVolumeDecrease10,        C_("Shortcut title", "Volume -10 Percent") },

      { {}, C_("Shortcut group title", "Tempo") },
      { settings::kKeyShortcutsTempoIncrease1,         C_("Shortcut title", "Tempo +1 Bpm") },
      { settings::kKeyShortcutsTempoDecrease1,         C_("Shortcut title", "Tempo -1 Bpm") },
      { settings::kKeyShortcutsTempoIncrease10,        C_("Shortcut title", "Tempo +10 Bpm") },
      { settings::kKeyShortcutsTempoDecrease10,        C_("Shortcut title", "Tempo -10 Bpm") },
      { settings::kKeyShortcutsTempoTap,               C_("Shortcut title", "Tempo Tap") },

      { {}, C_("Shortcut group title", "Accents") },
      { settings::kKeyShortcutsMeterEnabled,           C_("Shortcut title", "Enable / Disable Accentuation") },
      { settings::kKeyShortcutsMeterSelectSimple2,     C_("Shortcut title", "Select 2/4 Meter") },
      { settings::kKeyShortcutsMeterSelectSimple3,     C_("Shortcut title", "Select 3/4 Meter") },
      { settings::kKeyShortcutsMeterSelectSimple4,     C_("Shortcut title", "Select 4/4 Meter") },
      { settings::kKeyShortcutsMeterSelectCompound2,   C_("Shortcut title", "Select 6/8 Meter") },
      { settings::kKeyShortcutsMeterSelectCompound3,   C_("Shortcut title", "Select 9/8 Meter") },
      { settings::kKeyShortcutsMeterSelectCompound4,   C_("Shortcut title", "Select 12/8 Meter") },
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
    { kActionVolumeIncrease, Glib::Variant<double>::create(1) }
  },
  { settings::kKeyShortcutsVolumeDecrease1,
    { kActionVolumeDecrease, Glib::Variant<double>::create(1) }
  },
  { settings::kKeyShortcutsVolumeIncrease10,
    { kActionVolumeIncrease, Glib::Variant<double>::create(10) }
  },
  { settings::kKeyShortcutsVolumeDecrease10,
    { kActionVolumeDecrease, Glib::Variant<double>::create(10) }
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
  { settings::kKeyShortcutsMeterSelectSimple2,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterSimple2) }
  },
  { settings::kKeyShortcutsMeterSelectSimple3,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterSimple3) }
  },
  { settings::kKeyShortcutsMeterSelectSimple4,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterSimple4) }
  },
  { settings::kKeyShortcutsMeterSelectCompound2,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCompound2) }
  },
  { settings::kKeyShortcutsMeterSelectCompound3,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCompound3) }
  },
  { settings::kKeyShortcutsMeterSelectCompound4,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCompound4) }
  },
  { settings::kKeyShortcutsMeterSelectCustom,
    { kActionMeterSelect, Glib::Variant<Glib::ustring>::create(kActionMeterCustom) }
  }
};
