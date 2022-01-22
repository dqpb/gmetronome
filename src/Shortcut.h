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
  Glib::ustring title; // translated shortcut title
};

// Returns a list of grouped shortcut entries as they appear in the shortcuts
// tree view in the settings dialog. The group titles have an empty key.
const std::vector<ShortcutEntry>& ShortcutList();

struct ShortcutAction
{
  Glib::ustring action_name;
  Glib::VariantBase target_value;
};

// maps a settings key to a shortcut action
extern const std::map<Glib::ustring, ShortcutAction> kDefaultShortcutActionMap;

#endif//GMetronome_Shortcut_h
