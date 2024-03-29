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
  Glib::ustring key;   // settings key
  Glib::ustring title; // translated shortcut title
};

enum class ShortcutGroupIdentifier
{
  Application = 1,
  View        = 2,
  Transport   = 3,
  Tempo       = 4,
  Accents     = 5,
  Trainer     = 6,
  Pendulum    = 7,
  Volume      = 8
};

struct ShortcutGroup
{
  ShortcutGroupIdentifier group_id;        // group identifier
  Glib::ustring title;                     // translated group title
  std::vector<ShortcutEntry> shortcuts;    // list of shortcuts in the group
};

// Returns a list of grouped shortcut entries as they appear in the shortcuts
// tree view in the settings dialog or in the Gtk::ShortcutsWindow.
const std::vector<ShortcutGroup>& ShortcutList();

struct ShortcutAction
{
  Glib::ustring action_name;
  Glib::VariantBase target_value;
};

// maps a settings key to a shortcut action
extern const std::map<Glib::ustring, ShortcutAction> kDefaultShortcutActionMap;

#endif//GMetronome_Shortcut_h
