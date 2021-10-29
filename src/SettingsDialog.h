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

#ifndef __SETTINGS_DIALOG_H__
#define __SETTINGS_DIALOG_H__

#include "AccentButton.h"
#include <gtkmm.h>

/** 
 * Main settings dialog
 */
class SettingsDialog : public Gtk::Dialog
{
public:
  SettingsDialog(BaseObjectType* cobject,
		 const Glib::RefPtr<Gtk::Builder>& builder);
  
  static SettingsDialog* create(Gtk::Window& parent);
  
private:
  Glib::RefPtr<Gtk::Builder> builder_;
  
  // GSettings
  Glib::RefPtr<Gio::Settings> settings_;
  Glib::RefPtr<Gio::Settings> settings_prefs_;
  Glib::RefPtr<Gio::Settings> settings_shortcuts_;

  // General 
  Gtk::Switch* restore_profile_switch_;
  Gtk::ComboBoxText* animation_combo_box_;
  Gtk::SpinButton* animation_sync_spin_button_;
  Glib::RefPtr<Gtk::Adjustment> animation_sync_adjustment_;

  // Sound and Audio 
  Gtk::Grid* sound_grid_;
  Glib::RefPtr<Gtk::Adjustment> sound_high_freq_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_high_vol_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_freq_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_vol_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_low_freq_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_low_vol_adjustment_;

  AccentButtonDrawingArea high_accent_drawing_;
  AccentButtonDrawingArea mid_accent_drawing_;
  AccentButtonDrawingArea low_accent_drawing_;

  Gtk::ComboBoxText* audio_backend_combo_box_;

  // Shortcuts
  Gtk::TreeView* shortcuts_tree_view_;
  Gtk::Button* shortcuts_reset_button_;
  
  class ShortcutsModelColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ShortcutsModelColumns() { add(action_name); add(key); }
    
    Gtk::TreeModelColumn<Glib::ustring> action_name;
    Gtk::TreeModelColumn<Glib::ustring> key;
  } shortcuts_model_columns_;
  
  Glib::RefPtr<Gtk::TreeStore> shortcuts_tree_store_;
  Gtk::CellRendererAccel accel_cell_renderer_;
  
  void onAccelCellData(Gtk::CellRenderer* cell,
                       const Gtk::TreeModel::iterator& iter);

  void onAccelCleared(const Glib::ustring& path_string);
  
  void onAccelEdited(const Glib::ustring& path_string,
                     guint accel_key,
                     Gdk::ModifierType accel_mods,
                     guint hardware_keycode);

  void onResetShortcuts();
  void onAnimationSyncChanged();
  
  //void onSettingsPrefsChanged(const Glib::ustring& key);
  void onSettingsShortcutsChanged(const Glib::ustring& key);
};

#endif//__SETTINGS_DIALOG_H__
