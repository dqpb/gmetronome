/*
 * Copyright (C) 2020-2022 The GMetronome Team
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

// data model for sound theme TreeView
class SoundThemesModelColumns : public Gtk::TreeModel::ColumnRecord {
public:
  SoundThemesModelColumns()
    { add(title); add(id); add(settings); add(settings_connection); }

  Gtk::TreeModelColumn<Glib::ustring> id;
  Gtk::TreeModelColumn<Glib::ustring> title;
  Gtk::TreeModelColumn<Glib::RefPtr<Gio::Settings>> settings;
  Gtk::TreeModelColumn<sigc::connection> settings_connection;
};

// data model for shortcuts TreeView
class ShortcutsModelColumns : public Gtk::TreeModel::ColumnRecord {
public:
  ShortcutsModelColumns() { add(action_name); add(key); }

  Gtk::TreeModelColumn<Glib::ustring> action_name;
  Gtk::TreeModelColumn<Glib::ustring> key;
};

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

  Gtk::Notebook* main_notebook_;
  Gtk::Stack* main_stack_;

  // General tab
  Gtk::Switch* restore_profile_switch_;

  // Animation tab
  Gtk::ComboBoxText* pendulum_action_combo_box_;
  Gtk::ComboBoxText* pendulum_phase_mode_combo_box_;
  Gtk::Switch* accent_animation_switch_;
  Gtk::SpinButton* animation_sync_spin_button_;
  Glib::RefPtr<Gtk::Adjustment> animation_sync_adjustment_;

  // Sound tab
  Gtk::Grid* sound_grid_;
  Gtk::TreeView* sound_theme_tree_view_;
  SoundThemesModelColumns sound_theme_model_columns_;
  Glib::RefPtr<Gtk::ListStore> sound_theme_list_store_;
  Gtk::Button* sound_theme_add_button_;
  Gtk::Button* sound_theme_remove_button_;
  Gtk::RadioButton* sound_strong_radio_button_;
  Gtk::RadioButton* sound_mid_radio_button_;
  Gtk::RadioButton* sound_weak_radio_button_;
  Gtk::Scale* sound_timbre_scale_;
  Gtk::Switch* sound_bell_switch_;
  Gtk::Scale* sound_balance_scale_;
  Glib::RefPtr<Gtk::Adjustment> sound_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_bell_volume_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_balance_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> sound_volume_adjustment_;

  AccentButtonDrawingArea strong_accent_drawing_;
  AccentButtonDrawingArea mid_accent_drawing_;
  AccentButtonDrawingArea weak_accent_drawing_;

  Glib::ustring sound_theme_title_new_;
  Glib::ustring sound_theme_title_placeholder_;

  // Audio Device tab
  Gtk::ComboBoxText* audio_backend_combo_box_;
  Gtk::ComboBoxText* audio_device_combo_box_;
  Gtk::Entry* audio_device_entry_;
  Gtk::Spinner *audio_device_spinner_;

  // Shortcuts tab
  Gtk::TreeView* shortcuts_tree_view_;
  ShortcutsModelColumns shortcuts_model_columns_;
  Gtk::Button* shortcuts_reset_button_;
  Glib::RefPtr<Gtk::TreeStore> shortcuts_tree_store_;
  Gtk::CellRendererAccel accel_cell_renderer_;

  // Connections
  std::vector<sigc::connection> sound_theme_parameters_connections_;
  sigc::connection sound_theme_selection_changed_connection_;
  sigc::connection audio_device_entry_changed_connection_;

private:
  // Initialization
  void initActions();
  void initUI();
  void initBindings();

  // Callbacks
  bool onKeyPressEvent(GdkEventKey* key_event);
  void onAnimationSyncChanged();
  void onSoundThemeSelect();
  void onSoundThemeTitleStartEditing(Gtk::CellEditable* editable, const Glib::ustring& path);
  void onSoundThemeTitleChanged(const Glib::ustring& path, const Glib::ustring& new_text);
  void onSoundThemeAdd();
  void onSoundThemeRemove();
  void onSoundThemeAccentChanged();
  void onSoundThemeParametersChanged();
  void updateSoundThemeList();
  void updateSoundThemeSelection();
  void updateSoundThemeTitle(const Glib::ustring& theme_id);
  void updateSoundThemeParameters(const Glib::ustring& theme_id);
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
  void onSettingsSoundChanged(const Glib::ustring& key);
  void onSettingsSoundThemeChanged(const Glib::ustring& key, const Glib::ustring& theme_id);
  void onSettingsShortcutsChanged(const Glib::ustring& key);
  void onAppActionStateChanged(const Glib::ustring& action_name,
                               const Glib::VariantBase& variant);
};

#endif//GMetronome_SettingsDialog_h
