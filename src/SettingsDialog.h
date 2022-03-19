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

#ifndef GMetronome_SettingsDialog_h
#define GMetronome_SettingsDialog_h

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

  ~SettingsDialog();

  static SettingsDialog* create(Gtk::Window& parent);

private:
  Glib::RefPtr<Gtk::Builder> builder_;

  // GSettings
  Glib::RefPtr<Gio::Settings> settings_;
  Glib::RefPtr<Gio::Settings> settings_prefs_;
  Glib::RefPtr<Gio::Settings> settings_shortcuts_;

  // General tab
  Gtk::Switch* restore_profile_switch_;
  Gtk::ComboBoxText* pendulum_action_combo_box_;
  Gtk::ComboBoxText* pendulum_phase_mode_combo_box_;
  Gtk::Switch* accent_animation_switch_;
  Gtk::SpinButton* animation_sync_spin_button_;
  Glib::RefPtr<Gtk::Adjustment> animation_sync_adjustment_;

  // Sound tab
  Gtk::Grid* sound_grid_;
  Glib::RefPtr<Gtk::Adjustment> sound_strong_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_strong_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_strong_vol_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_strong_bal_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_vol_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_mid_bal_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_weak_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_weak_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_weak_vol_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_weak_bal_adjustment_;

  AccentButtonDrawingArea strong_accent_drawing_;
  AccentButtonDrawingArea mid_accent_drawing_;
  AccentButtonDrawingArea weak_accent_drawing_;

  // Audio Settings tab
  Gtk::ComboBoxText* audio_backend_combo_box_;
  Gtk::ComboBoxText* audio_device_combo_box_;
  Gtk::Entry* audio_device_entry_;
  Gtk::Spinner *audio_device_spinner_;

  sigc::connection audio_device_entry_changed_connection_;

  // Shortcuts tab
  class ShortcutsModelColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ShortcutsModelColumns() { add(action_name); add(key); }

    Gtk::TreeModelColumn<Glib::ustring> action_name;
    Gtk::TreeModelColumn<Glib::ustring> key;
  } shortcuts_model_columns_;

  Gtk::TreeView* shortcuts_tree_view_;
  Gtk::Button* shortcuts_reset_button_;
  Glib::RefPtr<Gtk::TreeStore> shortcuts_tree_store_;
  Gtk::CellRendererAccel accel_cell_renderer_;

  // Initialization
  void initActions();
  void initUI();
  void initBindings();

  // Callbacks
  bool onKeyPressEvent(GdkEventKey* key_event);

  void onAnimationSyncChanged();

  void onAudioDeviceEntryActivate();
  void onAudioDeviceEntryChanged();
  void onAudioDeviceEntryFocusIn();
  void onAudioDeviceEntryFocusOut();
  void onAudioDeviceChanged();
  void updateAudioDeviceList();
  void updateAudioDevice();

  void onAccelCellData(Gtk::CellRenderer* cell,
                       const Gtk::TreeModel::iterator& iter);

  void onAccelCleared(const Glib::ustring& path_string);

  void onAccelEdited(const Glib::ustring& path_string,
                     guint accel_key,
                     Gdk::ModifierType accel_mods,
                     guint hardware_keycode);

  void onResetShortcuts();

  void onSettingsPrefsChanged(const Glib::ustring& key);
  void onSettingsShortcutsChanged(const Glib::ustring& key);
  void onAppActionStateChanged(const Glib::ustring& action_name,
                               const Glib::VariantBase& variant);
};

#endif//GMetronome_SettingsDialog_h
