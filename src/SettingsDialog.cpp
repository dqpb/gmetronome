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

#include "Application.h"
#include "SettingsDialog.h"
#include "Settings.h"
#include "Shortcut.h"
#include "AudioBackend.h"
#include <glibmm/i18n.h>
#include <iostream>

SettingsDialog::SettingsDialog(BaseObjectType* cobject,
			       const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Dialog(cobject),
  builder_(builder)
{
  builder_->get_widget("animationComboBox", animation_combo_box_);
  builder_->get_widget("animationSyncSpinButton", animation_sync_spin_button_);  
  builder_->get_widget("restoreProfileSwitch", restore_profile_switch_);  
  builder_->get_widget("audioBackendComboBox", audio_backend_combo_box_);
  builder_->get_widget("audioDeviceComboBox", audio_device_combo_box_);
  builder_->get_widget("audioDeviceEntry", audio_device_entry_);
  builder_->get_widget("audioDeviceSpinner", audio_device_spinner_);
  builder_->get_widget("shortcutsResetButton", shortcuts_reset_button_);  
  builder_->get_widget("shortcutsTreeView", shortcuts_tree_view_);
  builder_->get_widget("soundGrid", sound_grid_);  
  
  animation_sync_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("animationSyncAdjustment"));
  sound_strong_freq_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundStrongFreqAdjustment"));
  sound_strong_vol_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundStrongVolAdjustment"));
  sound_strong_bal_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundStrongBalAdjustment"));
  sound_mid_freq_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundMidFreqAdjustment"));
  sound_mid_vol_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundMidVolAdjustment"));
  sound_mid_bal_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundMidBalAdjustment"));
  sound_weak_freq_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundWeakFreqAdjustment"));
  sound_weak_vol_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundWeakVolAdjustment"));
  sound_weak_bal_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundWeakBalAdjustment"));  

  initSettings();
  initActions();
  initUI();
  initBindings();
}

//static
SettingsDialog* SettingsDialog::create(Gtk::Window& parent)
{
  // load the Builder file and instantiate its widgets
  auto builder = Gtk::Builder::create_from_resource("/org/gmetronome/ui/SettingsDialog.glade");

  SettingsDialog* dialog = nullptr;
  builder->get_widget_derived("settingsDialog", dialog);
  if (!dialog)
    throw std::runtime_error("No \"settingsDialog\" object in SettingsDialog.glade");

  dialog->set_transient_for(parent);
  return dialog;
}

void SettingsDialog::initSettings()
{
  settings_ = Gio::Settings::create(settings::kSchemaId);
  settings_prefs_ = settings_->get_child(settings::kSchemaIdPrefsBasename);
  settings_shortcuts_ = settings_prefs_->get_child(settings::kSchemaIdShortcutsBasename);
}

void SettingsDialog::initActions() {}

void SettingsDialog::initUI()
{
  // init sound section
  strong_accent_drawing_.setAccentState(kAccentStrong);
  mid_accent_drawing_.setAccentState(kAccentMid);
  weak_accent_drawing_.setAccentState(kAccentWeak);

  strong_accent_drawing_.show();
  mid_accent_drawing_.show();
  weak_accent_drawing_.show();

  sound_grid_->attach(strong_accent_drawing_, 1, 1);
  sound_grid_->attach(mid_accent_drawing_, 2, 1);
  sound_grid_->attach(weak_accent_drawing_, 3, 1);  

  // init audio device section
  // remove unavailable audio backends from combo box
  const auto& backends = audio::availableBackends();
  auto n_backends = audio_backend_combo_box_->get_model()->children().size();
  for (int index = n_backends - 1; index >= 0; --index)
  {
    if (std::find(backends.begin(), backends.end(), index) == backends.end())
      audio_backend_combo_box_->remove_text(index);
  }
  updateAudioDeviceList();
  updateAudioDevice();

  audio_device_spinner_->stop();

  // shortcuts section
  shortcuts_tree_store_ = Gtk::TreeStore::create(shortcuts_model_columns_);

  Gtk::TreeIter top_iter;
  for (const auto& entry : ShortcutList())
  {
    Gtk::TreeIter iter;

    if (entry.key.empty())
      top_iter = iter = shortcuts_tree_store_->append();
    else if (top_iter)
      iter = shortcuts_tree_store_->append(top_iter->children());
    else
      continue;
    
    auto row = *iter;
    
    row[shortcuts_model_columns_.action_name] = entry.title;
    row[shortcuts_model_columns_.key] = entry.key;
  }

  shortcuts_tree_view_->append_column(
    // Shortcuts table header title (first column)
    _("Action"), 
    shortcuts_model_columns_.action_name );
  
  shortcuts_tree_view_->insert_column_with_data_func(
    -1,
    // Shortcuts table header title (second column)
    _("Shortcut"),
    accel_cell_renderer_,
    sigc::mem_fun(*this, &SettingsDialog::onAccelCellData) );
  
  shortcuts_tree_view_->get_column(0)->set_expand();
  shortcuts_tree_view_->set_model(shortcuts_tree_store_);
  shortcuts_tree_view_->expand_all();
}

void SettingsDialog::initBindings()
{
  auto app = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  settings_prefs_->bind(settings::kKeyPrefsMeterAnimation, animation_combo_box_->property_active_id());
  settings_prefs_->bind(settings::kKeyPrefsAudioBackend, audio_backend_combo_box_->property_active_id());
  settings_prefs_->bind(settings::kKeyPrefsAnimationSync, animation_sync_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundStrongFrequency, sound_strong_freq_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundStrongVolume, sound_strong_vol_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundStrongBalance, sound_strong_bal_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundMidFrequency, sound_mid_freq_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundMidVolume, sound_mid_vol_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundMidBalance, sound_mid_bal_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundWeakFrequency, sound_weak_freq_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundWeakVolume, sound_weak_vol_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsSoundWeakBalance, sound_weak_bal_adjustment_->property_value());
  settings_prefs_->bind(settings::kKeyPrefsRestoreProfile, restore_profile_switch_, "state", Gio::SETTINGS_BIND_DEFAULT);

  settings_prefs_->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsPrefsChanged));

  settings_shortcuts_->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsShortcutsChanged));

  app->signal_action_state_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAppActionStateChanged));

  animation_sync_spin_button_->signal_value_changed()
    .connect( sigc::mem_fun(*this, &SettingsDialog::onAnimationSyncChanged) );

  // audio device section
  audio_device_connections.push_back(
    audio_device_combo_box_->property_active_id().signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAudioDeviceChanged) )
  );
  
  audio_device_entry_->add_events(Gdk::FOCUS_CHANGE_MASK);
    
  audio_device_connections.push_back(
    audio_device_entry_->signal_activate()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAudioDeviceEntryActivate) )
  );

  audio_device_connections.push_back(
    audio_device_entry_->signal_focus_out_event()
    .connect( [this] (GdkEventFocus* gdk_event)->bool
      {
        this->onAudioDeviceEntryFocusOut();
        return false;
      })
  ); 

  // shortcuts section
  shortcuts_reset_button_->signal_clicked()
    .connect( sigc::mem_fun(*this, &SettingsDialog::onResetShortcuts) );

  accel_cell_renderer_.signal_accel_cleared()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAccelCleared));

  accel_cell_renderer_.signal_accel_edited()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAccelEdited));
}

void SettingsDialog::onAnimationSyncChanged()
{
  if (animation_sync_spin_button_->get_value() != 0)
    animation_sync_spin_button_
      ->set_icon_from_icon_name("dialog-warning", Gtk::ENTRY_ICON_PRIMARY);
  else
    animation_sync_spin_button_->unset_icon(Gtk::ENTRY_ICON_PRIMARY);
}

void SettingsDialog::onAudioDeviceEntryActivate()
{
  audio_backend_combo_box_->grab_focus();
  //onAudioDeviceChanged();
}

void SettingsDialog::onAudioDeviceEntryFocusOut()
{
  onAudioDeviceChanged();
}

void SettingsDialog::onAudioDeviceChanged()
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings_prefs_->get_enum(settings::kKeyPrefsAudioBackend);

  if (auto it = settings::kBackendToDeviceMap.find(backend);
      it != settings::kBackendToDeviceMap.end())
  {
    settings_prefs_->set_string(it->second, audio_device_entry_->get_text());
  }
}

void SettingsDialog::updateAudioDeviceList()
{
  auto app = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  std::vector<Glib::ustring> dev_list;
  app->get_action_state(kActionAudioDeviceList, dev_list);

  audio_device_combo_box_->remove_all();

  for (auto& dev : dev_list)
  {
    if (!dev.empty())
      audio_device_combo_box_->append(dev, dev);
  }
}

void SettingsDialog::updateAudioDevice()
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings_prefs_->get_enum(settings::kKeyPrefsAudioBackend);
  
  if (auto it = settings::kBackendToDeviceMap.find(backend);
      it != settings::kBackendToDeviceMap.end())
  {
    auto device = settings_prefs_->get_string(it->second);
    
    for ( auto& c : audio_device_connections )
      c.block();

    if (!audio_device_combo_box_->set_active_id(device))
      audio_device_entry_->set_text(device);

    for ( auto& c : audio_device_connections )
      c.unblock();    
  }
  else audio_device_entry_->set_text("");
}

void SettingsDialog::onAccelCellData(Gtk::CellRenderer* cell,
                                     const Gtk::TreeModel::iterator& iter)
{
  auto row = *iter;
  const Glib::ustring& key = row[shortcuts_model_columns_.key];
  
  auto accel_cell = static_cast<Gtk::CellRendererAccel*>(cell);
  
  if (key.empty())
  {
    // title row
    accel_cell->property_accel_key() = 0;
    accel_cell->property_accel_mods() = (Gdk::ModifierType) 0;
    accel_cell->property_visible() = false;
    accel_cell->property_sensitive() = false;
    accel_cell->property_editable() = false;
  }
  else
  {
    Glib::ustring accel = settings_shortcuts_->get_string(key);
    
    guint accel_key;
    GdkModifierType accel_mods;
    
    gtk_accelerator_parse(accel.c_str(), &accel_key, &accel_mods);
    
    bool writable = settings_shortcuts_->is_writable(key);
    
    Glib::Variant<Glib::ustring> variant;
    
    bool custom = settings_shortcuts_->get_user_value(key, variant);
    
    accel_cell->property_accel_key() = accel_key;
    accel_cell->property_accel_mods() = (Gdk::ModifierType) accel_mods;
    accel_cell->property_visible() = true;
    accel_cell->property_sensitive() = writable;
    accel_cell->property_editable() = writable;
    
    if (custom)
      accel_cell->property_weight() = Pango::WEIGHT_BOLD;
    else
      accel_cell->property_weight() = Pango::WEIGHT_NORMAL;
  }
}

void SettingsDialog::onAccelCleared(const Glib::ustring& path_string)
{
  onAccelEdited(path_string, 0, Gdk::ModifierType(0), 0);
}
  
void SettingsDialog::onAccelEdited(const Glib::ustring& path_string,
                                   guint accel_key,
                                   Gdk::ModifierType accel_mods,
                                   guint hardware_keycode)
{
  auto iter = shortcuts_tree_store_->get_iter(path_string);
  if (iter)
  {
    auto row = *iter;
    const Glib::ustring& key = row[shortcuts_model_columns_.key];

    if (!key.empty())
    {
      Glib::ustring accel = gtk_accelerator_name(accel_key, (GdkModifierType) accel_mods);
      settings_shortcuts_->set_string(key, accel);
    }
  }
}

void SettingsDialog::onResetShortcuts()
{
  for (const auto& entry  : ShortcutList())
    if (!entry.key.empty())
      settings_shortcuts_->reset(entry.key);
}

void SettingsDialog::onSettingsPrefsChanged(const Glib::ustring& key)
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings_prefs_->get_enum(settings::kKeyPrefsAudioBackend);

  if (key == settings::kKeyPrefsAudioBackend)
  {
    updateAudioDevice();
  }
  if (auto it = settings::kDeviceToBackendMap.find(key);
      it != settings::kDeviceToBackendMap.end() && backend == it->second)
  {
    updateAudioDevice();
  }
}

void SettingsDialog::onSettingsShortcutsChanged(const Glib::ustring& key)
{
  auto foreach_slot = [this, &key] (const Gtk::TreeModel::iterator& iter) -> bool
    {
      auto row = *iter;
      const auto& row_key = row[shortcuts_model_columns_.key];

      if (row_key == key)
      {
        shortcuts_tree_store_->row_changed( Gtk::TreePath(iter), iter);
        return true;
      }
      else
        return false;
    };

  shortcuts_tree_store_->foreach_iter(foreach_slot);
}

void SettingsDialog::onAppActionStateChanged(const Glib::ustring& action_name,
                                             const Glib::VariantBase& variant)
{
  if (action_name == kActionAudioDeviceList)
  {
    updateAudioDeviceList();
  }
}
