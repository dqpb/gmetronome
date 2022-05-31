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
#include <map>

class SoundThemeEditor;

// data model for sound theme TreeView
class SoundThemeModelColumns : public Gtk::TreeModel::ColumnRecord {
public:
  SoundThemeModelColumns()
    { add(type); add(id); add(title); add(settings); add(settings_connection); }

  enum class Type { kHeadline, kSeparator, kPreset, kCustom };

  Gtk::TreeModelColumn<Type> type;
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

  // General tab
  Gtk::Switch* restore_profile_switch_;
  Gtk::Switch* save_sound_theme_switch_;

  // Animation tab
  Gtk::ComboBoxText* pendulum_action_combo_box_;
  Gtk::ComboBoxText* pendulum_phase_mode_combo_box_;
  Gtk::Switch* accent_animation_switch_;
  Gtk::SpinButton* animation_sync_spin_button_;
  Glib::RefPtr<Gtk::Adjustment> animation_sync_adjustment_;

  // Sound tab
  Gtk::Grid* sound_grid_;
  Gtk::TreeView* sound_theme_tree_view_;
  SoundThemeModelColumns sound_theme_model_columns_;
  Glib::RefPtr<Gtk::TreeStore> sound_theme_tree_store_;
  Gtk::Button* sound_theme_add_button_;
  Gtk::Button* sound_theme_remove_button_;
  Gtk::Button* sound_theme_edit_button_;

  Glib::ustring sound_theme_title_new_;
  Glib::ustring sound_theme_title_duplicate_;
  Glib::ustring sound_theme_title_placeholder_;

  // sound theme editor dialogs
  std::map<Glib::ustring, SoundThemeEditor*> sound_theme_editors_;

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
  sigc::connection sound_theme_settings_list_connection_;
  sigc::connection sound_theme_selection_changed_connection_;
  sigc::connection audio_device_entry_changed_connection_;

private:
  // Initialization
  void initActions();
  void initUI();
  void initBindings();

  // Callbacks
  bool onKeyPressEvent(GdkEventKey* key_event);
  void onHideSoundThemeEditor(const Glib::ustring& id);
  void onAnimationSyncChanged();
  void onSoundThemeSelect();
  void onSoundThemeTitleStartEditing(Gtk::CellEditable* editable, const Glib::ustring& path);
  void onSoundThemeTitleChanged(const Glib::ustring& path, const Glib::ustring& new_text);
  void onSoundThemeAdd();
  void onSoundThemeRemove();
  void onSoundThemeEdit();

  void updateSoundThemeModelRows(const Gtk::TreeModel::Children& rows,
                                 const std::vector<Glib::ustring>& themes,
                                 const SoundThemeModelColumns::Type& type);

  void updateSoundThemeTreeStore();
  void updateSoundThemeSelection();
  void updateSoundThemeTitle(const Glib::ustring& theme_id);
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
