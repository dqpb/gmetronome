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
      // Group title for shortcuts regarding application actions
      { {}, C_("Shortcut group title", "Application") },
        
      { settings::kKeyShortcutsShowPrimaryMenu,       C_("Shortcut title", "Show Primary Menu") },
      { settings::kKeyShortcutsShowProfiles,          C_("Shortcut title", "Show Profiles") },
      { settings::kKeyShortcutsShowPreferences,       C_("Shortcut title", "Show Preferences") },
      { settings::kKeyShortcutsShowShortcuts,         C_("Shortcut title", "Show Keyboard Shortcuts") },
      { settings::kKeyShortcutsShowHelp,              C_("Shortcut title", "Show Help") },
      { settings::kKeyShortcutsShowAbout,             C_("Shortcut title", "Show About") },
      { settings::kKeyShortcutsQuit,                  C_("Shortcut title", "Quit") },
        
      // Group title for shortcuts regarding view actions
      { {}, C_("Shortcut group title", "View") },
        
      { settings::kKeyShortcutsShowMeter,             C_("Shortcut title", "Show Accents") },
      { settings::kKeyShortcutsShowTrainer,           C_("Shortcut title", "Show Trainer") },
      { settings::kKeyShortcutsFullScreen,            C_("Shortcut title", "Full Screen") },
        
      // Group title for shortcuts regarding transport actions
      { {}, C_("Shortcut group title", "Transport") },

      { settings::kKeyShortcutsStart,                  C_("Shortcut title", "Start / Stop") },
        
      // Group title for shortcuts regarding volume actions
      { {}, C_("Shortcut group title", "Volume") },

      { settings::kKeyShortcutsVolumeIncrease1,         C_("Shortcut title", "Volume +1") },
      { settings::kKeyShortcutsVolumeDecrease1,         C_("Shortcut title", "Volume -1") },
      { settings::kKeyShortcutsVolumeIncrease10,        C_("Shortcut title", "Volume +10") },
      { settings::kKeyShortcutsVolumeDecrease10,        C_("Shortcut title", "Volume -10") },

      // Group title for shortcuts regarding tempo actions
      { {}, C_("Shortcut group title", "Tempo") },

      { settings::kKeyShortcutsTempoIncrease1,         C_("Shortcut title", "Tempo +1") },
      { settings::kKeyShortcutsTempoDecrease1,         C_("Shortcut title", "Tempo -1") },
      { settings::kKeyShortcutsTempoIncrease10,        C_("Shortcut title", "Tempo +10") },
      { settings::kKeyShortcutsTempoDecrease10,        C_("Shortcut title", "Tempo -10") },
      { settings::kKeyShortcutsTempoTap,               C_("Shortcut title", "Tempo Tap") },
        
      // Group title for shortcuts regarding accent actions
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
        
      // Group title for shortcuts regarding trainer actions
      { {}, C_("Shortcut group title", "Trainer") },
        
      { settings::kKeyShortcutsTrainerEnabled,         C_("Shortcut title", "Enable / Disable Trainer") }
    };

  return kShortcutList;
}
