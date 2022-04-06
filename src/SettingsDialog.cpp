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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Application.h"
#include "SettingsDialog.h"
#include "Settings.h"
#include "Shortcut.h"
#include "AudioBackend.h"

#include <glibmm/i18n.h>
#include <cassert>
#include <iostream>

SettingsDialog::SettingsDialog(BaseObjectType* cobject,
                               const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Dialog(cobject),
  builder_(builder)
{
  builder_->get_widget("mainNotebook", main_notebook_);
  builder_->get_widget("mainStack", main_stack_);
  builder_->get_widget("pendulumActionComboBox", pendulum_action_combo_box_);
  builder_->get_widget("pendulumPhaseModeComboBox", pendulum_phase_mode_combo_box_);
  builder_->get_widget("accentAnimationSwitch", accent_animation_switch_);
  builder_->get_widget("animationSyncSpinButton", animation_sync_spin_button_);
  builder_->get_widget("restoreProfileSwitch", restore_profile_switch_);
  builder_->get_widget("soundGrid", sound_grid_);
  builder_->get_widget("soundThemeTreeView", sound_theme_tree_view_);
  builder_->get_widget("soundThemeAddButton", sound_theme_add_button_);
  builder_->get_widget("soundThemeRemoveButton", sound_theme_remove_button_);
  builder_->get_widget("soundStrongRadioButton", sound_strong_radio_button_);
  builder_->get_widget("soundMidRadioButton", sound_mid_radio_button_);
  builder_->get_widget("soundWeakRadioButton", sound_weak_radio_button_);
  builder_->get_widget("soundBellSwitch", sound_bell_switch_);
  builder_->get_widget("soundBalanceScale", sound_balance_scale_);
  builder_->get_widget("audioBackendComboBox", audio_backend_combo_box_);
  builder_->get_widget("audioDeviceComboBox", audio_device_combo_box_);
  builder_->get_widget("audioDeviceEntry", audio_device_entry_);
  builder_->get_widget("audioDeviceSpinner", audio_device_spinner_);
  builder_->get_widget("shortcutsResetButton", shortcuts_reset_button_);
  builder_->get_widget("shortcutsTreeView", shortcuts_tree_view_);

  animation_sync_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("animationSyncAdjustment"));
  sound_timbre_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundTimbreAdjustment"));
  sound_pitch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundPitchAdjustment"));
  sound_bell_volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundBellVolumeAdjustment"));
  sound_balance_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundBalanceAdjustment"));
  sound_volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("soundVolumeAdjustment"));

  initActions();
  initUI();
  initBindings();
}

SettingsDialog::~SettingsDialog() {}

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
  // Gtk::Notebook seems to slow down the ui of the child pages, which is really
  // noticeable when sliding a Gtk::Scale. So we just use the 'nice' page switcher
  // with some dummy pages and connect our page stack to the signal_switch_page().

  main_notebook_->signal_switch_page().connect(
    [&] (Widget* widget, guint page) {
      switch (page)
      {
      case 0: main_stack_->set_visible_child("general"); break;
      case 1: main_stack_->set_visible_child("animation"); break;
      case 2: main_stack_->set_visible_child("sound"); break;
      case 3: main_stack_->set_visible_child("audio"); break;
      case 4: main_stack_->set_visible_child("shortcuts"); break;
      default:
        break;
      };
    });

  //
  // Sound tab
  //
  sound_theme_title_new_ = gettext(SoundTheme::kDefaultTitle.c_str());
  sound_theme_title_placeholder_ = gettext(SoundTheme::kDefaultTitlePlaceholder.c_str());

  sound_theme_list_store_ = Gtk::ListStore::create(sound_theme_model_columns_);
  sound_theme_tree_view_->set_model(sound_theme_list_store_);
  sound_theme_tree_view_->append_column_editable("Title", sound_theme_model_columns_.title);

  strong_accent_drawing_.setAccentState(kAccentStrong);
  mid_accent_drawing_.setAccentState(kAccentMid);
  weak_accent_drawing_.setAccentState(kAccentWeak);

  strong_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);
  mid_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);
  weak_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);

  strong_accent_drawing_.show();
  mid_accent_drawing_.show();
  weak_accent_drawing_.show();

  sound_strong_radio_button_->add(strong_accent_drawing_);
  sound_mid_radio_button_->add(mid_accent_drawing_);
  sound_weak_radio_button_->add(weak_accent_drawing_);

  sound_balance_scale_->add_mark(0.0, Gtk::POS_BOTTOM, "");

  updateSoundThemeList();

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

  settings::soundThemeList()->settings()->signal_changed()
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

  sound_strong_radio_button_->signal_clicked().connect(
    [this] () { if (sound_strong_radio_button_->get_active()) onSoundThemeAccentChanged(); });
  sound_mid_radio_button_->signal_clicked().connect(
    [this] () { if (sound_mid_radio_button_->get_active()) onSoundThemeAccentChanged(); });
  sound_weak_radio_button_->signal_clicked().connect(
    [this] () { if (sound_weak_radio_button_->get_active()) onSoundThemeAccentChanged(); });

  sound_theme_parameters_connections_.reserve(6);

  sound_theme_parameters_connections_.push_back(
    sound_timbre_adjustment_->signal_value_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

  sound_theme_parameters_connections_.push_back(
    sound_pitch_adjustment_->signal_value_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

  sound_theme_parameters_connections_.push_back(
    sound_bell_switch_->property_state().signal_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

  sound_theme_parameters_connections_.push_back(
    sound_bell_volume_adjustment_->signal_value_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

  sound_theme_parameters_connections_.push_back(
    sound_balance_adjustment_->signal_value_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

  sound_theme_parameters_connections_.push_back(
    sound_volume_adjustment_->signal_value_changed()
    .connect(sigc::mem_fun(*this, &SettingsDialog::onSoundThemeParametersChanged)));

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
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
  Glib::ustring id;
  auto row_it = sound_theme_tree_view_->get_selection()->get_selected();

  if (row_it)
    id = row_it->get_value(sound_theme_model_columns_.id);

  settings::soundThemeList()->select(id);
}

void SettingsDialog::onSoundThemeTitleStartEditing(Gtk::CellEditable* editable,
                                                   const Glib::ustring& path)
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
  // nothing to do here
}

void SettingsDialog::onSoundThemeTitleChanged(const Glib::ustring& path,
                                              const Glib::ustring& new_text)
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  if (auto rowit = sound_theme_list_store_->get_iter(path); rowit)
  {
    const auto& col_settings = sound_theme_model_columns_.settings;
    if (auto theme_settings = rowit->get_value(col_settings); theme_settings)
    {
      theme_settings->set_string(settings::kKeySoundThemeTitle, new_text);
    }
  }
}

void SettingsDialog::onSoundThemeAdd()
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  try {
    // duplicate the selected sound theme
    auto id = settings::soundThemeList()->selected();
    if (!id.empty())
    {
      auto theme = settings::soundThemeList()->get(id);
      theme.title = sound_theme_title_new_;
      settings::soundThemeList()->append(theme);
    }
    else settings::soundThemeList()->append({});
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
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
  auto id = settings::soundThemeList()->selected();
  settings::soundThemeList()->remove(id);
}

void SettingsDialog::onSoundThemeAccentChanged()
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  const auto& col_id  = sound_theme_model_columns_.id;

  if (auto selected_row_it = sound_theme_tree_view_->get_selection()->get_selected();
      selected_row_it)
  {
    updateSoundThemeParameters(selected_row_it->get_value(col_id));
  }
}

void SettingsDialog::onSoundThemeParametersChanged()
{
  //std::cerr << __PRETTY_FUNCTION__ << std::endl;

  const auto& col_id  = sound_theme_model_columns_.id;
  const auto& col_settings = sound_theme_model_columns_.settings;
  const auto& col_settings_connection = sound_theme_model_columns_.settings_connection;

  if (auto selected_row_it = sound_theme_tree_view_->get_selection()->get_selected();
      selected_row_it)
  {
    if (auto theme_id = selected_row_it->get_value(col_id);
        !theme_id.empty())
    {
      settings::SoundParametersTuple params =
        {
          sound_timbre_adjustment_->get_value(),
          sound_pitch_adjustment_->get_value(),
          sound_bell_switch_->get_state(),
          sound_bell_volume_adjustment_->get_value(),
          sound_balance_adjustment_->get_value(),
          sound_volume_adjustment_->get_value()
        };

      auto value = Glib::Variant<settings::SoundParametersTuple>::create(params);
      auto theme_settings = selected_row_it->get_value(col_settings);

      selected_row_it->get_value(col_settings_connection).block();

      if (sound_strong_radio_button_->get_active())
        theme_settings->set_value(settings::kKeySoundThemeStrongParams, value);
      else if (sound_mid_radio_button_->get_active())
        theme_settings->set_value(settings::kKeySoundThemeMidParams, value);
      else if (sound_weak_radio_button_->get_active())
        theme_settings->set_value(settings::kKeySoundThemeWeakParams, value);

      selected_row_it->get_value(col_settings_connection).unblock();
    }
  }
}

void SettingsDialog::updateSoundThemeList()
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  auto& col_id  = sound_theme_model_columns_.id;
  auto& col_title = sound_theme_model_columns_.title;
  auto& col_settings = sound_theme_model_columns_.settings;
  auto& col_settings_connection = sound_theme_model_columns_.settings_connection;

  sound_theme_selection_changed_connection_.block();

  auto themes = settings::soundThemeList()->list();
  auto children = sound_theme_list_store_->children();
  auto rowit = children.begin();

  for (const auto& id : themes)
  {
    if (rowit == children.end() || rowit->get_value(col_id) != id)
    {
      auto tmp_rowit =
        std::find_if(rowit, children.end(),
                     [&col_id,&id = id] (const auto& row) {
                       return row[col_id] == id;
                     });
      if (tmp_rowit != children.end())
      {
        sound_theme_list_store_->move(tmp_rowit, rowit);
        rowit = tmp_rowit;
      }
      else
        rowit = sound_theme_list_store_->insert(rowit);
    }

    // update row
    auto row = *rowit;
    row[col_id] = id;

    auto theme_settings = settings::soundThemeList()->settings(id);

    // disconnect the old theme before updating settings object
    rowit->get_value(col_settings_connection).disconnect();
    row[col_settings] = theme_settings;

    if (theme_settings)
    {
      row[col_title] = theme_settings->get_string(settings::kKeySoundThemeTitle);
      row[col_settings_connection] =  theme_settings->signal_changed()
        .connect([=] (const Glib::ustring& key) { onSettingsSoundThemeChanged(key, id); });
    }
    else
    {
      row[col_title] = "";
    }
    ++rowit;
  }

  while (rowit != children.end())
  {
    rowit->get_value(col_settings_connection).disconnect();
    rowit = sound_theme_list_store_->erase(rowit);
  }
  sound_theme_selection_changed_connection_.unblock();

  updateSoundThemeSelection();
}

void SettingsDialog::updateSoundThemeSelection()
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  auto theme_id = settings::soundThemeList()->selected();
  auto rows = sound_theme_list_store_->children();
  auto it = std::find_if(rows.begin(), rows.end(),
                         [this, &theme_id] (const auto& row) -> bool {
                           return row[sound_theme_model_columns_.id] == theme_id;
                         });

  sound_theme_selection_changed_connection_.block();
  {
    if (it!=rows.end())
      sound_theme_tree_view_->get_selection()->select(it);
    else
      sound_theme_tree_view_->get_selection()->unselect_all();
  }
  sound_theme_selection_changed_connection_.unblock();

  updateSoundThemeParameters(theme_id);
}

void SettingsDialog::updateSoundThemeTitle(const Glib::ustring& theme_id)
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  const auto& col_id  = sound_theme_model_columns_.id;
  const auto& col_title = sound_theme_model_columns_.title;
  const auto& col_settings = sound_theme_model_columns_.settings;

  auto rows = sound_theme_list_store_->children();
  auto it = std::find_if(rows.begin(), rows.end(),
                         [&col_id, &theme_id] (const auto& row) -> bool {
                           return row[col_id] == theme_id;
                         });
  if (it != rows.end())
  {
    if (auto theme_settings = it->get_value(col_settings);
        theme_settings)
    {
      auto new_title = theme_settings->get_string(settings::kKeySoundThemeTitle);
      it->set_value(col_title, new_title);
    }
  }
}

void SettingsDialog::updateSoundThemeParameters(const Glib::ustring& theme_id)
{
  std::cerr << __PRETTY_FUNCTION__ << std::endl;

  const auto& col_id  = sound_theme_model_columns_.id;

  if (auto selected_row_it = sound_theme_tree_view_->get_selection()->get_selected();
      selected_row_it && selected_row_it->get_value(col_id) == theme_id)
  {
    try {
      auto sound_theme = settings::soundThemeList()->get(theme_id);

      auto& sound_params =
          sound_strong_radio_button_->get_active() ? sound_theme.strong_params
        : sound_mid_radio_button_->get_active() ? sound_theme.mid_params
        : sound_theme.weak_params;

      for (auto& connection : sound_theme_parameters_connections_)
        connection.block();

      sound_timbre_adjustment_->set_value(sound_params.timbre);
      sound_pitch_adjustment_->set_value(sound_params.pitch);
      sound_bell_switch_->set_state(sound_params.bell);
      sound_bell_volume_adjustment_->set_value(sound_params.bell_volume);
      sound_balance_adjustment_->set_value(sound_params.balance);
      sound_volume_adjustment_->set_value(sound_params.volume);

      for (auto& connection : sound_theme_parameters_connections_)
        connection.unblock();
    }
    catch (...)
    {
#ifndef NDEBUG
      std::cerr << "SettingsDialog: failed to update sound theme parameters"
                << std::endl;
#endif
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
  for (const auto& entry  : ShortcutList())
    if (!entry.key.empty())
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
  std::cout << __PRETTY_FUNCTION__ << " key: " << key << std::endl;

  if (key == settings::kKeySoundVolume)
  {}
  else if (key == settings::kKeySettingsListEntries)
  {
    updateSoundThemeList();
  }
  else if (key == settings::kKeySettingsListSelectedEntry)
  {
    updateSoundThemeSelection();
  }
}

void SettingsDialog::onSettingsSoundThemeChanged(const Glib::ustring& key,
                                                 const Glib::ustring& theme_id)
{
  std::cout << __PRETTY_FUNCTION__ << " key: " << key << std::endl;

  if (key == settings::kKeySoundThemeTitle)
  {
    updateSoundThemeTitle(theme_id);
  }
  else if (key == settings::kKeySoundThemeStrongParams
           || key == settings::kKeySoundThemeMidParams
           || key == settings::kKeySoundThemeWeakParams)
  {
    updateSoundThemeParameters(theme_id);
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
