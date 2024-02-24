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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Application.h"
#include "SettingsDialog.h"
#include "Settings.h"
#include "Shortcut.h"
#include "AudioBackend.h"
#include "SoundThemeEditor.h"
#include "MainWindow.h"

#include <glibmm/i18n.h>
#include <cassert>
#include <iostream>

SettingsDialog::SettingsDialog(BaseObjectType* cobject,
                               const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Dialog(cobject),
  builder_(builder)
{
  builder_->get_widget("mainNotebook", main_notebook_);
  builder_->get_widget("pendulumActionComboBox", pendulum_action_combo_box_);
  builder_->get_widget("pendulumPhaseModeComboBox", pendulum_phase_mode_combo_box_);
  builder_->get_widget("accentAnimationSwitch", accent_animation_switch_);
  builder_->get_widget("animationSyncSpinButton", animation_sync_spin_button_);
  builder_->get_widget("restoreProfileSwitch", restore_profile_switch_);
  builder_->get_widget("linkSoundThemeSwitch", link_sound_theme_switch_);
  builder_->get_widget("autoAdjustVolumeSwitch", auto_adjust_volume_switch_);
  builder_->get_widget("soundGrid", sound_grid_);
  builder_->get_widget("soundThemeTreeView", sound_theme_tree_view_);
  builder_->get_widget("soundThemeAddButton", sound_theme_add_button_);
  builder_->get_widget("soundThemeRemoveButton", sound_theme_remove_button_);
  builder_->get_widget("soundThemeEditButton", sound_theme_edit_button_);
  builder_->get_widget("audioBackendComboBox", audio_backend_combo_box_);
  builder_->get_widget("audioDeviceComboBox", audio_device_combo_box_);
  builder_->get_widget("audioDeviceEntry", audio_device_entry_);
  builder_->get_widget("audioDeviceSpinner", audio_device_spinner_);
  builder_->get_widget("shortcutsResetButton", shortcuts_reset_button_);
  builder_->get_widget("shortcutsTreeView", shortcuts_tree_view_);

  animation_sync_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("animationSyncAdjustment"));

  initActions();
  initUI();
  initBindings();
}

SettingsDialog::~SettingsDialog()
{
  // delete remaining sound theme editors
  for(auto& [id, editor_ptr] : sound_theme_editors_)
    delete editor_ptr;
}

//static
SettingsDialog* SettingsDialog::create(Gtk::Window& parent)
{
  // load the Builder file and instantiate its widgets
  auto win_resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/ui/SettingsDialog.glade";
  auto builder = Gtk::Builder::create_from_resource(win_resource_path);

  SettingsDialog* dialog = nullptr;
  builder->get_widget_derived("settingsDialog", dialog);
  if (!dialog)
    throw std::runtime_error("No \"settingsDialog\" object in SettingsDialog.glade");

  dialog->set_transient_for(parent);
  return dialog;
}

void SettingsDialog::initActions() {}

void SettingsDialog::initUI()
{
  //
  // Sound tab
  //
  sound_theme_title_new_ = gettext(SoundTheme::kDefaultTitle.c_str());
  sound_theme_title_placeholder_ = gettext(SoundTheme::kDefaultTitlePlaceholder.c_str());
  sound_theme_title_duplicate_ = gettext(SoundTheme::kDefaultTitleDuplicate.c_str());

  sound_theme_tree_store_ = Gtk::TreeStore::create(sound_theme_model_columns_);
  sound_theme_tree_view_->set_model(sound_theme_tree_store_);
  sound_theme_tree_view_->append_column_editable("Sound Theme", sound_theme_model_columns_.title);

  sound_theme_tree_view_->set_row_separator_func(
    [&] (const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::iterator& rowit)
      {
        SoundThemeModelColumns::Type type = rowit->get_value(sound_theme_model_columns_.type);
        return (type == SoundThemeModelColumns::Type::kSeparator);
      });

  sound_theme_tree_view_->get_selection()->set_select_function(
    [&] (const Glib::RefPtr<Gtk::TreeModel>& model,
         const Gtk::TreeModel::Path& path,
         bool path_currently_selected)
      {
        if (auto rowit = model->get_iter(path); rowit)
        {
          SoundThemeModelColumns::Type type = rowit->get_value(sound_theme_model_columns_.type);
          return (type == SoundThemeModelColumns::Type::kPreset)
            || (type == SoundThemeModelColumns::Type::kCustom);
        }
        return false;
      });

  auto cell_renderer = dynamic_cast<Gtk::CellRendererText*>(
    sound_theme_tree_view_->get_column_cell_renderer(0));

  auto title_column = sound_theme_tree_view_->get_column(0);

  title_column->set_cell_data_func(
    *cell_renderer, [&] (Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& rowit)
      {
        if (auto type = rowit->get_value(sound_theme_model_columns_.type);
            type == SoundThemeModelColumns::Type::kCustom)
          cell->set_property("editable", true);
        else
          cell->set_property("editable", false);

        // if (auto type = rowit->get_value(sound_theme_model_columns_.type);
        //     type == SoundThemeModelColumns::Type::kHeadline)
        //   cell->set_property("weight", Pango::WEIGHT_SEMIBOLD);
        // else
        //   cell->set_property("weight", Pango::WEIGHT_NORMAL);
      });

  updateSoundThemeTreeStore();
  sound_theme_tree_view_->expand_all();
  updateSoundThemeSelection();

  //
  // Audio device tab
  //
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

  //
  // Shortcuts tab
  //
  shortcuts_tree_store_ = Gtk::TreeStore::create(shortcuts_model_columns_);

  for (const auto& group : ShortcutList())
  {
    Gtk::TreeIter group_iter = shortcuts_tree_store_->append();

    (*group_iter)[shortcuts_model_columns_.action_name] = group.title;
    (*group_iter)[shortcuts_model_columns_.key] = "";

    for (const auto& entry : group.shortcuts)
    {
      Gtk::TreeIter entry_iter = shortcuts_tree_store_->append(group_iter->children());

      (*entry_iter)[shortcuts_model_columns_.action_name] = entry.title;
      (*entry_iter)[shortcuts_model_columns_.key] = entry.key;
    }
  }
  shortcuts_tree_view_->append_column(
    // Shortcuts table header title (first column)
    C_("Preferences dialog", "Action"),
    shortcuts_model_columns_.action_name );

  shortcuts_tree_view_->insert_column_with_data_func(
    -1,
    // Shortcuts table header title (second column)
    C_("Preferences dialog", "Shortcut"),
    accel_cell_renderer_,
    sigc::mem_fun(*this, &SettingsDialog::onAccelCellData) );

  shortcuts_tree_view_->get_column(0)->set_expand();
  shortcuts_tree_view_->set_model(shortcuts_tree_store_);
  shortcuts_tree_view_->expand_all();
}


void SettingsDialog::initBindings()
{
  auto app = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  signal_key_press_event()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onKeyPressEvent));

  settings::preferences()->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsPrefsChanged));

  app->signal_action_state_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAppActionStateChanged));

  //
  // General tab
  //
  settings::preferences()->bind(settings::kKeyPrefsRestoreProfile,
                                restore_profile_switch_->property_state());
  settings::preferences()->bind(settings::kKeyPrefsLinkSoundTheme,
                                link_sound_theme_switch_->property_state());
  settings::sound()->bind(settings::kKeySoundAutoAdjustVolume,
                          auto_adjust_volume_switch_->property_state());
  //
  // Animation tab
  //
  settings::preferences()->bind(settings::kKeyPrefsMeterAnimation,
                                accent_animation_switch_->property_state());
  settings::preferences()->bind(settings::kKeyPrefsPendulumAction,
                                pendulum_action_combo_box_->property_active_id());
  settings::preferences()->bind(settings::kKeyPrefsPendulumPhaseMode,
                                pendulum_phase_mode_combo_box_->property_active_id());

  animation_sync_spin_button_->signal_value_changed()
    .connect( sigc::mem_fun(*this, &SettingsDialog::onAnimationSyncChanged) );

  settings::preferences()->bind(settings::kKeyPrefsAnimationSync,
                                animation_sync_adjustment_->property_value());
  //
  // Sound tab
  //
  settings::sound()->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsSoundChanged));

  sound_theme_settings_list_connection_ =
    settings::soundThemes()->settings()->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsSoundChanged));

  sound_theme_selection_changed_connection_ =
    sound_theme_tree_view_->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeSelect));

  auto cell_renderer = dynamic_cast<Gtk::CellRendererText*>(
    sound_theme_tree_view_->get_column_cell_renderer(0));

  cell_renderer->property_placeholder_text() = sound_theme_title_placeholder_;

  cell_renderer->signal_editing_started()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeTitleStartEditing));

  cell_renderer->signal_edited()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeTitleChanged));

  sound_theme_add_button_->signal_clicked()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeAdd));
  sound_theme_remove_button_->signal_clicked()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeRemove));
  sound_theme_edit_button_->signal_clicked()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeEdit));

  //
  // Audio device tab
  //
  settings::preferences()->bind(settings::kKeyPrefsAudioBackend,
                                audio_backend_combo_box_->property_active_id());

  audio_device_entry_->add_events(Gdk::FOCUS_CHANGE_MASK);

  audio_device_entry_->signal_activate()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAudioDeviceEntryActivate));

  audio_device_entry_changed_connection_ =
    audio_device_entry_->property_text().signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAudioDeviceEntryChanged));

  audio_device_entry_->signal_focus_out_event()
    .connect( [this] (GdkEventFocus* gdk_event)->bool
      {
        this->onAudioDeviceEntryFocusOut();
        return false;
      });

  audio_device_entry_->signal_focus_in_event()
    .connect( [this] (GdkEventFocus* gdk_event)->bool
      {
        this->onAudioDeviceEntryFocusIn();
        return false;
      });

  //
  // Shortcuts tab
  //
  settings::shortcuts()->signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSettingsShortcutsChanged));
  shortcuts_reset_button_->signal_clicked()
    .connect( sigc::mem_fun(*this, &SettingsDialog::onResetShortcuts) );
  accel_cell_renderer_.signal_accel_cleared()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAccelCleared));
  accel_cell_renderer_.signal_accel_edited()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onAccelEdited));
}

bool SettingsDialog::onKeyPressEvent(GdkEventKey* event)
{
  switch (event->keyval) {
  case GDK_KEY_Escape:
  {
    close();
    return TRUE;
  }
  default:
    return FALSE;
  };
}

void SettingsDialog::onHideSoundThemeEditor(const Glib::ustring& id)
{
  if (auto it = sound_theme_editors_.find(id); it != sound_theme_editors_.end())
  {
    delete it->second;
    sound_theme_editors_.erase(it);
  }
#ifndef NDEBUG
  else {
    std::cerr << "SettingsDialog: could not delete unregistered sound theme editor (id:"
              << "'" << id <<"')" << std::endl;
  }
#endif
}

void SettingsDialog::onAnimationSyncChanged()
{
  if (animation_sync_spin_button_->get_value() != 0)
    animation_sync_spin_button_
      ->set_icon_from_icon_name("dialog-warning", Gtk::ENTRY_ICON_PRIMARY);
  else
    animation_sync_spin_button_->unset_icon(Gtk::ENTRY_ICON_PRIMARY);
}

void SettingsDialog::onSoundThemeSelect()
{
  Glib::ustring id;
  auto row_it = sound_theme_tree_view_->get_selection()->get_selected();

  if (row_it)
    id = row_it->get_value(sound_theme_model_columns_.id);

  settings::soundThemes()->select(id);
}

void SettingsDialog::onSoundThemeTitleStartEditing(Gtk::CellEditable* editable,
                                                   const Glib::ustring& path)
{
  // nothing to do here
}

void SettingsDialog::onSoundThemeTitleChanged(const Glib::ustring& path,
                                              const Glib::ustring& new_text)
{
  if (auto rowit = sound_theme_tree_store_->get_iter(path); rowit)
  {
    const auto& col_settings = sound_theme_model_columns_.settings;
    if (auto theme_settings = rowit->get_value(col_settings); theme_settings)
    {
      theme_settings->set_string(settings::kKeySoundThemeTitle, new_text);
    }
  }
}

// helper
template<class ColumnType, class ValueType>
Gtk::TreeModel::iterator findRowRecursively(Glib::RefPtr<Gtk::TreeStore>& store,
                                            const Gtk::TreeModelColumn<ColumnType>& column,
                                            ValueType& value)
{
  Gtk::TreeModel::iterator rowit;
  store->foreach_iter( [&] (const Gtk::TreeModel::iterator& it)
    {
      if (it->get_value(column) == value)
      {
        rowit = it;
        return true;
      }
      else return false;
    });
  return rowit;
}

void SettingsDialog::onSoundThemeAdd()
{
  try {
    sound_theme_settings_list_connection_.block();
    auto theme_id = settings::soundThemes()->selected();
    if (!theme_id.empty())
    {
      // duplicate the selected sound theme
      auto theme = settings::soundThemes()->get(theme_id);

      theme.title = MainWindow::duplicateDocumentTitle(theme.title,
                                                       sound_theme_title_duplicate_,
                                                       sound_theme_title_placeholder_);

      theme_id = settings::soundThemes()->append(theme);
    }
    else
    {
      SoundTheme new_theme {sound_theme_title_new_};
      theme_id = settings::soundThemes()->append(new_theme);
    }

    // select new theme
    settings::soundThemes()->select(theme_id);

    // update ui
    updateSoundThemeTreeStore();
    updateSoundThemeSelection();

    if (auto rowit = findRowRecursively(sound_theme_tree_store_,
                                        sound_theme_model_columns_.id,
                                        theme_id); rowit)
    {
      sound_theme_tree_view_->grab_focus();
      sound_theme_tree_view_->set_cursor(sound_theme_tree_store_->get_path(rowit),
                                         *(sound_theme_tree_view_->get_column(0)),
                                         true);
    }
    sound_theme_settings_list_connection_.unblock();
  }
  catch (...)
  {
#ifndef NDEBUG
    std::cerr << "SettingsDialog: could not create new sound theme" << std::endl;
#endif
  }
}

void SettingsDialog::onSoundThemeRemove()
{
  const auto& col_id  = sound_theme_model_columns_.id;
  const auto& col_type  = sound_theme_model_columns_.type;
  const auto& col_settings_connection = sound_theme_model_columns_.settings_connection;

  if (auto rowit = sound_theme_tree_view_->get_selection()->get_selected(); rowit)
  {
    if (auto id = rowit->get_value(col_id); !id.empty())
    {
      if (auto type = rowit->get_value(col_type);
          type == SoundThemeModelColumns::Type::kCustom)
        {
          sound_theme_settings_list_connection_.block();
          sound_theme_selection_changed_connection_.block();

          auto settings_connection = rowit->get_value(col_settings_connection);
          settings_connection.disconnect();

          Glib::ustring next_id = "";

          auto prev = rowit;
          auto next = rowit;

          if (++next && next->get_value(col_type) == SoundThemeModelColumns::Type::kCustom)
            next_id = next->get_value(col_id);
          else if (--prev && prev->get_value(col_type) == SoundThemeModelColumns::Type::kCustom)
            next_id = prev->get_value(col_id);
          else
            next_id = settings::soundThemes()->defaults().front();

          settings::soundThemes()->remove(id);
          updateSoundThemeTreeStore();

          sound_theme_settings_list_connection_.unblock();
          sound_theme_selection_changed_connection_.unblock();

          settings::soundThemes()->select(next_id);
        }
    }
  }
}

void SettingsDialog::onSoundThemeEdit()
{
  const auto& col_id  = sound_theme_model_columns_.id;
  const auto& col_type  = sound_theme_model_columns_.type;

  if (auto rowit = sound_theme_tree_view_->get_selection()->get_selected(); rowit)
  {
    if (auto id = rowit->get_value(col_id); !id.empty())
    {
      if (auto type = rowit->get_value(col_type);
          type == SoundThemeModelColumns::Type::kCustom)
      {
        if (auto it = sound_theme_editors_.find(id); it != sound_theme_editors_.end())
        {
          it->second->present();
        }
        else
        {
          auto new_editor = SoundThemeEditor::create(*this, id);
          sound_theme_editors_[id] = new_editor;
          new_editor->signal_hide().connect([this,id]{onHideSoundThemeEditor(id);});
          new_editor->present();
        }
      }
    }
  }
}

//helper
void SettingsDialog::updateSoundThemeModelRows(const Gtk::TreeModel::Children& rows,
                                               const std::vector<Glib::ustring>& themes,
                                               const SoundThemeModelColumns::Type& type)
{
  auto& col_type  = sound_theme_model_columns_.type;
  auto& col_id    = sound_theme_model_columns_.id;
  auto& col_title = sound_theme_model_columns_.title;
  auto& col_settings = sound_theme_model_columns_.settings;
  auto& col_settings_connection = sound_theme_model_columns_.settings_connection;

  auto rowit = rows.begin();

  // update rows
  for (const auto& id : themes)
  {
    if (rowit == rows.end())
      rowit = sound_theme_tree_store_->insert(rowit);

    auto theme_settings = settings::soundThemes()->settings(id).settings;

    // disconnect the old theme before updating
    rowit->get_value(col_settings_connection).disconnect();

    auto row = *rowit;
    row[col_type] = type;
    row[col_id] = id;
    row[col_settings] = theme_settings;

    if (theme_settings)
    {
      row[col_title] = theme_settings->get_string(settings::kKeySoundThemeTitle);
      row[col_settings_connection] =  theme_settings->signal_changed()
        .connect([=] (const Glib::ustring& key) { onSettingsSoundThemeChanged(key, id); });
    }
    else {
      row[col_title] = "";
    }

    ++rowit;
  }

  // remove remaining rows
  while (rowit != rows.end())
  {
    rowit->get_value(sound_theme_model_columns_.settings_connection).disconnect();
    rowit = sound_theme_tree_store_->erase(rowit);
  }
}

void SettingsDialog::updateSoundThemeTreeStore()
{
  sound_theme_selection_changed_connection_.block();

  auto& col_type = sound_theme_model_columns_.type;
  auto& col_id = sound_theme_model_columns_.id;
  auto& col_title = sound_theme_model_columns_.title;

  auto top_rows = sound_theme_tree_store_->children();
  auto top_rowit = top_rows.begin();

  // update presets
  if (auto presets = settings::soundThemes()->defaults(); !presets.empty())
  {
    if (top_rowit == top_rows.end())
      top_rowit = sound_theme_tree_store_->append();

    top_rowit->set_value(col_type, SoundThemeModelColumns::Type::kHeadline);
    top_rowit->set_value(col_id, Glib::ustring(""));
    top_rowit->set_value(col_title, Glib::ustring(C_("Preferences dialog", "Presets")));

    updateSoundThemeModelRows(top_rowit->children(),
                              presets, SoundThemeModelColumns::Type::kPreset);
    ++top_rowit;
  }

  // update custom themes
  if (auto themes = settings::soundThemes()->list(false); !themes.empty())
  {
    if (top_rowit == top_rows.end())
      top_rowit = sound_theme_tree_store_->append();

    top_rowit->set_value(col_type, SoundThemeModelColumns::Type::kHeadline);
    top_rowit->set_value(col_id, Glib::ustring(""));
    top_rowit->set_value(col_title, Glib::ustring(C_("Preferences dialog", "Custom")));

    updateSoundThemeModelRows(top_rowit->children(),
                              themes, SoundThemeModelColumns::Type::kCustom);
    ++top_rowit;
  }

  // remove remaining toplevel rows
  while (top_rowit != top_rows.end())
  {
    top_rowit->get_value(sound_theme_model_columns_.settings_connection).disconnect();
    top_rowit = sound_theme_tree_store_->erase(top_rowit);
  }

  sound_theme_selection_changed_connection_.unblock();
}

void SettingsDialog::updateSoundThemeSelection()
{
  const auto& col_type = sound_theme_model_columns_.type;
  const auto& col_id = sound_theme_model_columns_.id;
  const auto theme_id = settings::soundThemes()->selected();

  if (auto rowit = findRowRecursively(sound_theme_tree_store_, col_id, theme_id);
      !theme_id.empty() && rowit)
  {
    auto path = sound_theme_tree_store_->get_path(rowit);
    sound_theme_tree_view_->expand_to_path(path);
    sound_theme_tree_view_->scroll_to_row(path);

    sound_theme_selection_changed_connection_.block();
    sound_theme_tree_view_->get_selection()->select(rowit);
    sound_theme_selection_changed_connection_.unblock();

    if (rowit->get_value(col_type) == SoundThemeModelColumns::Type::kCustom)
    {
      sound_theme_remove_button_->set_sensitive(true);
      sound_theme_edit_button_->set_sensitive(true);
    }
    else {
      sound_theme_remove_button_->set_sensitive(false);
      sound_theme_edit_button_->set_sensitive(false);
    }
  }
  else {
    sound_theme_selection_changed_connection_.block();
    sound_theme_tree_view_->get_selection()->unselect_all();
    sound_theme_selection_changed_connection_.unblock();

    sound_theme_remove_button_->set_sensitive(false);
    sound_theme_edit_button_->set_sensitive(false);
  }
}

void SettingsDialog::updateSoundThemeTitle(const Glib::ustring& theme_id)
{
  const auto& col_id  = sound_theme_model_columns_.id;
  const auto& col_title = sound_theme_model_columns_.title;
  const auto& col_settings = sound_theme_model_columns_.settings;

  if (auto rowit = findRowRecursively(sound_theme_tree_store_, col_id, theme_id);
      !theme_id.empty() && rowit)
  {
    if (auto theme_settings = rowit->get_value(col_settings); theme_settings)
    {
      auto new_title = theme_settings->get_string(settings::kKeySoundThemeTitle);
      rowit->set_value(col_title, new_title);
    }
  }
}

void SettingsDialog::onAudioDeviceEntryActivate()
{
  audio_backend_combo_box_->grab_focus();
}

void SettingsDialog::onAudioDeviceEntryChanged()
{
  onAudioDeviceChanged();
}

void SettingsDialog::onAudioDeviceEntryFocusIn()
{
  audio_device_entry_changed_connection_.block();
}

void SettingsDialog::onAudioDeviceEntryFocusOut()
{
  onAudioDeviceChanged();
  audio_device_entry_changed_connection_.unblock();
}

void SettingsDialog::onAudioDeviceChanged()
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings::preferences()->get_enum(settings::kKeyPrefsAudioBackend);

  if (auto it = settings::kBackendToDeviceMap.find(backend);
      it != settings::kBackendToDeviceMap.end())
  {
    settings::preferences()->set_string(it->second, audio_device_entry_->get_text());
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
    settings::preferences()->get_enum(settings::kKeyPrefsAudioBackend);

  if (auto it = settings::kBackendToDeviceMap.find(backend);
      it != settings::kBackendToDeviceMap.end())
  {
    auto device = settings::preferences()->get_string(it->second);

    audio_device_entry_changed_connection_.block();

    if (!audio_device_combo_box_->set_active_id(device))
      audio_device_entry_->set_text(device);

    audio_device_entry_changed_connection_.unblock();
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
    Glib::ustring accel = settings::shortcuts()->get_string(key);

    guint accel_key;
    GdkModifierType accel_mods;

    gtk_accelerator_parse(accel.c_str(), &accel_key, &accel_mods);

    bool writable = settings::shortcuts()->is_writable(key);

    Glib::Variant<Glib::ustring> variant;

    bool custom = settings::shortcuts()->get_user_value(key, variant);

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
      settings::shortcuts()->set_string(key, accel);
    }
  }
}

void SettingsDialog::onResetShortcuts()
{
  for (const auto& group  : ShortcutList())
    for (const auto& entry  : group.shortcuts)
      settings::shortcuts()->reset(entry.key);
}

void SettingsDialog::onSettingsPrefsChanged(const Glib::ustring& key)
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings::preferences()->get_enum(settings::kKeyPrefsAudioBackend);

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

void SettingsDialog::onSettingsSoundChanged(const Glib::ustring& key)
{
  if (key == settings::kKeySoundVolume)
  {}
  else if (key == settings::kKeySettingsListEntries)
  {
    updateSoundThemeTreeStore();
    updateSoundThemeSelection();
  }
  else if (key == settings::kKeySettingsListSelectedEntry)
  {
    updateSoundThemeSelection();
  }
}

void SettingsDialog::onSettingsSoundThemeChanged(const Glib::ustring& key,
                                                 const Glib::ustring& theme_id)
{
  if (key == settings::kKeySoundThemeTitle)
  {
    updateSoundThemeTitle(theme_id);
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
