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

#include "MainWindow.h"
#include "Application.h"
#include "ActionBinding.h"
#include "ProfilesListStore.h"
#include "SettingsDialog.h"
#include "AccentButton.h"
#include "Settings.h"
#include "Shortcut.h"
#include "config.h"

#include <glibmm/i18n.h>

#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <algorithm>

//static
MainWindow* MainWindow::create()
{
  // Load the Builder file and instantiate its widgets.
  auto builder_ = Gtk::Builder::create_from_resource("/org/gmetronome/ui/MainWindow.glade");

  MainWindow* window = nullptr;
  builder_->get_widget_derived("mainWindow", window);
  if (!window)
    throw std::runtime_error("No \"app_window\" object in window.ui");

  return window;
}

MainWindow::MainWindow(BaseObjectType* cobject,
                       const Glib::RefPtr<Gtk::Builder>& builder)
  : Gtk::ApplicationWindow(cobject),
    builder_(builder),
    shortcuts_window_(nullptr),
    animation_sync_usecs_(0)
{
  builder_->get_widget("headerBar", header_bar_);
  builder_->get_widget("tempoIntegralLabel", tempo_integral_label_);
  builder_->get_widget("tempoFractionLabel", tempo_fraction_label_);
  builder_->get_widget("tempoDividerLabel", tempo_divider_label_);
  builder_->get_widget("currentProfileLabel", current_profile_label_);
  builder_->get_widget("fullScreenButton", full_screen_button_);
  builder_->get_widget("fullScreenImage", full_screen_image_);
  builder_->get_widget("menuButton", menu_button_);
  builder_->get_widget("popoverMenu", popover_menu_);
  builder_->get_widget("profilesButton", profiles_button_);
  builder_->get_widget("profilesPopover", profiles_popover_);
  builder_->get_widget("profilesTreeView", profiles_tree_view_);
  builder_->get_widget("profilesNewButton", profiles_new_button_);
  builder_->get_widget("profilesDeleteButton", profiles_delete_button_);
  builder_->get_widget("mainBox", main_box_);
  builder_->get_widget("infoOverlay", info_overlay_);
  builder_->get_widget("infoRevealer", info_revealer_);
  builder_->get_widget("infoBar", info_bar_);
  builder_->get_widget("infoContentBox", info_content_box_);
  builder_->get_widget("infoButtonBox", info_button_box_);
  builder_->get_widget("infoImage", info_image_);
  builder_->get_widget("infoLabelBox", info_label_box_);
  builder_->get_widget("infoTopicLabel", info_topic_label_);
  builder_->get_widget("infoTextLabel", info_text_label_);
  builder_->get_widget("infoDetailsLabel", info_details_label_);
  builder_->get_widget("infoDetailsExpander", info_details_expander_);
  builder_->get_widget("contentBox", content_box_);
  builder_->get_widget("volumeButton", volume_button_);
  builder_->get_widget("startButton", start_button_);
  builder_->get_widget("trainerToggleButtonRevealer", trainer_toggle_button_revealer_);
  builder_->get_widget("accentToggleButtonRevealer", accent_toggle_button_revealer_);
  builder_->get_widget("trainerToggleButton", trainer_toggle_button_);
  builder_->get_widget("accentToggleButton", accent_toggle_button_);
  builder_->get_widget("trainerRevealer", trainer_revealer_);
  builder_->get_widget("accentRevealer", accent_revealer_);
  builder_->get_widget("trainerFrame", trainer_frame_);
  builder_->get_widget("accentFrame", accent_frame_);
  builder_->get_widget("accentBox", accent_box_);
  builder_->get_widget("tempoScale", tempo_scale_);
  builder_->get_widget("meterComboBox", meter_combo_box_);
  builder_->get_widget("beatsSpinButton", beats_spin_button_);
  builder_->get_widget("beatsLabel", beats_label_);
  builder_->get_widget("subdivComboBox", subdiv_combo_box_);
  builder_->get_widget("subdivLabel", subdiv_label_);

  tempo_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("tempoAdjustment"));

  trainer_start_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerStartAdjustment"));

  trainer_target_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerTargetAdjustment"));

  trainer_accel_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerAccelAdjustment"));

  beats_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("beatsAdjustment"));

  accent_button_grid_.show();
  accent_box_->pack_start(accent_button_grid_);

  profiles_list_store_ = ProfilesListStore::create();
  profiles_tree_view_->set_model(profiles_list_store_);
  profiles_tree_view_->append_column_editable("Title", profiles_list_store_->columns_.title_);
  //profiles_tree_view_->set_reorderable();
  profiles_tree_view_->enable_model_drag_source();
  profiles_tree_view_->enable_model_drag_dest();

  preferences_dialog_ = SettingsDialog::create(*this);

  initSettings();
  initActions();
  initUI();
  initAbout();
  initBindings();

  updatePrefAnimationSync();
  updatePrefMeterAnimation();
}

void MainWindow::initSettings()
{
  settings_ = Gio::Settings::create(settings::kSchemaId);
  settings_prefs_ = settings_->get_child(settings::kSchemaIdPrefsBasename);
  settings_state_ = settings_->get_child(settings::kSchemaIdStateBasename);
  settings_shortcuts_ = settings_prefs_->get_child(settings::kSchemaIdShortcutsBasename);

  settings_prefs_->signal_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onSettingsPrefsChanged));
}

void MainWindow::initActions()
{
  const ActionHandlerMap kWinActionHandler =
    {
      {kActionShowPrimaryMenu,         sigc::mem_fun(*this, &MainWindow::onShowPrimaryMenu)},
      {kActionShowProfiles,            sigc::mem_fun(*this, &MainWindow::onShowProfiles)},
      {kActionShowPreferences,         sigc::mem_fun(*this, &MainWindow::onShowPreferences)},
      {kActionShowShortcuts,           sigc::mem_fun(*this, &MainWindow::onShowShortcuts)},
      {kActionShowHelp,                sigc::mem_fun(*this, &MainWindow::onShowHelp)},
      {kActionShowAbout,               sigc::mem_fun(*this, &MainWindow::onShowAbout)},
      {kActionShowMeter,               settings_state_},
      {kActionShowTrainer,             settings_state_},
      {kActionFullScreen,              sigc::mem_fun(*this, &MainWindow::onToggleFullScreen)}
    };

  install_actions(*this, kActionDescriptions, kWinActionHandler);
}

void MainWindow::initUI()
{
  using std::literals::chrono_literals::operator""us;

  // initialize title bar
  titlebar_bin_.add(*header_bar_);
  set_titlebar(titlebar_bin_);
  titlebar_bin_.show();

  // initialize header bar
  updateCurrentTempo( { 0us, 0us, { 0, 0, -1, 0us } } );

  // initialize info bar
  info_overlay_->add_overlay(*info_revealer_);
  info_revealer_->set_reveal_child(false);

  // initialize tempo interface
  Glib::ustring mark_30 = Glib::ustring::format(30);
  Glib::ustring mark_120= Glib::ustring::format(120);
  Glib::ustring mark_250 = Glib::ustring::format(250);

  tempo_scale_->add_mark(30.0, Gtk::POS_BOTTOM, mark_30);
  tempo_scale_->add_mark(120.0, Gtk::POS_BOTTOM, mark_120);
  tempo_scale_->add_mark(250.0, Gtk::POS_BOTTOM, mark_250);

  tempo_scale_->set_round_digits(0);

  auto app = Gtk::Application::get_default();

  // initialize meter interface
  Glib::ustring meter_slot;
  app->get_action_state(kActionMeterSelect, meter_slot);

  Meter meter;
  app->get_action_state(meter_slot, meter);

  updateMeterInterface(meter_slot, meter);

  // initialize profiles list
  ProfilesList list;
  app->get_action_state(kActionProfilesList, list);

  updateProfilesList(list);

  // initalize profile selection
  Glib::ustring id;
  app->get_action_state(kActionProfilesSelect, id);

  updateProfilesSelect(id);

  // initialize profiles title
  Glib::ustring title;
  app->get_action_state(kActionProfilesTitle, title);

  updateProfilesTitle(title);
}

void MainWindow::initAbout()
{
  about_dialog_.set_program_name(Glib::get_application_name());
  about_dialog_.set_version(VERSION);
  about_dialog_.set_license_type(Gtk::LICENSE_GPL_3_0);
  //about_dialog_.set_authors({"The GMetronome Team"});

  //Translators: Put one translator per line, in the form
  //NAME <EMAIL>, YEAR1, YEAR2
  about_dialog_.set_translator_credits(C_("About dialog", "translator-credits"));

  static const int year = 2021;

  auto copyright = Glib::ustring::compose(
    //Parameters:
    // %1 - year of the last commit
    // %2 - localized application name
    C_("About dialog", "Copyright © 2020-%1 The %2 Team"),
    year, Glib::get_application_name());

  about_dialog_.set_copyright(copyright);

  //about_dialog_.set_website("http://");
  //about_dialog_.set_website_label( about_dialog_.get_website() );

  about_dialog_.set_logo(
    Gdk::Pixbuf::create_from_resource("/org/gmetronome/icons/scalable/gmetronome.svg",128,128));

}

void MainWindow::initBindings()
{
  auto app = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  settings_prefs_->bind(settings::kKeyPrefsVolume,
                        volume_button_,
                        "value",
                        Gio::SETTINGS_BIND_DEFAULT);

  settings_state_->bind(settings::kKeyStateShowMeter,
                        accent_revealer_,
                        "reveal-child",
                        Gio::SETTINGS_BIND_GET);

  settings_state_->bind(settings::kKeyStateShowMeter,
                        accent_toggle_button_revealer_,
                        "reveal-child",
                        Gio::SETTINGS_BIND_GET);

  settings_state_->bind(settings::kKeyStateShowTrainer,
                        trainer_revealer_,
                        "reveal-child",
                        Gio::SETTINGS_BIND_GET);

  settings_state_->bind(settings::kKeyStateShowTrainer,
                        trainer_toggle_button_revealer_,
                        "reveal-child",
                        Gio::SETTINGS_BIND_GET);

  bindings_
    .push_back( Glib::Binding::bind_property( trainer_toggle_button_->property_active(),
                                              trainer_frame_->property_sensitive() ));
  bindings_
    .push_back( Glib::Binding::bind_property( accent_toggle_button_->property_active(),
                                              accent_frame_->property_sensitive() ));
  action_bindings_
    .push_back( bind_action(app,
                            kActionTempo,
                            tempo_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app,
                            kActionTrainerStart,
                            trainer_start_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app,
                            kActionTrainerTarget,
                            trainer_target_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app,
                            kActionTrainerAccel,
                            trainer_accel_adjustment_->property_value()) );

  meter_connections_
    .push_back( beats_adjustment_->signal_value_changed()
                .connect(sigc::mem_fun(*this, &MainWindow::onBeatsChanged))
      );
  meter_connections_
    .push_back( meter_combo_box_->signal_changed()
                .connect(sigc::mem_fun(*this, &MainWindow::onMeterChanged))
      );
  meter_connections_
    .push_back( subdiv_combo_box_->signal_changed()
                .connect(sigc::mem_fun(*this, &MainWindow::onSubdivChanged))
      );
  meter_connections_
    .push_back( accent_button_grid_.signal_accent_changed()
                .connect(sigc::mem_fun(*this, &MainWindow::onAccentChanged))
      );

  profiles_tree_view_->signal_drag_begin()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesDragBegin));

  profiles_tree_view_->signal_drag_end()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesDragEnd));

  profiles_selection_changed_connection_ =
    profiles_tree_view_->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesSelectionChanged));

  auto cell_renderer = dynamic_cast<Gtk::CellRendererText*>(
    profiles_tree_view_->get_column_cell_renderer(0));

  profiles_title_changed_connection_ =
    cell_renderer->signal_editing_started()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesTitleStartEditing));

  profiles_title_changed_connection_ =
    cell_renderer->signal_edited()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesTitleChanged));

  app->signal_action_state_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onActionStateChanged));

  profiles_popover_->signal_show()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesShow));

  profiles_popover_->signal_hide()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfilesHide));

  app->signal_message()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessage));

  info_bar_->signal_response()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessageResponse));

  app->signal_ticker_statistics()
    .connect(sigc::mem_fun(*this, &MainWindow::onTickerStatistics));

  tempo_integral_label_->signal_size_allocate()
    .connect(sigc::mem_fun(*this, &MainWindow::onTempoLabelAllocate));
}

MainWindow::~MainWindow()
{
  if (preferences_dialog_ != nullptr)
    delete preferences_dialog_;

  if (shortcuts_window_ != nullptr)
    delete shortcuts_window_;
}

bool MainWindow::on_window_state_event(GdkEventWindowState* window_state_event)
{
  Gtk::ApplicationWindow::on_window_state_event(window_state_event);

  if (window_state_event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
  {
    bool new_state;
    if (window_state_event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
    {
      header_bar_->reparent(*main_box_);
      header_bar_->set_decoration_layout(":minimize,close");
      main_box_->reorder_child(*header_bar_, 0);
      full_screen_image_->set_from_icon_name("view-restore-symbolic",
                                             Gtk::ICON_SIZE_BUTTON);
      full_screen_button_->show();
      new_state = true;
    }
    else
    {
      header_bar_->reparent(titlebar_bin_);
      header_bar_->unset_decoration_layout();
      full_screen_image_->set_from_icon_name("view-fullscreen-symbolic",
                                             Gtk::ICON_SIZE_BUTTON);
      full_screen_button_->hide();
      new_state = false;
    }

    Glib::RefPtr<Gio::Action> action = lookup_action(kActionFullScreen);
    auto simple_action =  Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
    simple_action->set_state(Glib::Variant<bool>::create(new_state));
  }
  return true;
}

void MainWindow::onTempoLabelAllocate(Gtk::Allocation& alloc)
{
  // make sure that the header bar is always high enough
  // for the tempo label and the profile name label
  header_bar_->set_size_request(-1, 2 * alloc.get_height() + 10);
}

void MainWindow::onProfilesShow()
{
  profiles_tree_view_->set_can_focus(true);

  if (profiles_tree_view_->get_selection()->count_selected_rows() != 0)
    profiles_tree_view_->property_has_focus() = true;
  else
    profiles_new_button_->property_has_focus() = true;
}

void MainWindow::onProfilesHide()
{
  profiles_tree_view_->set_can_focus(false);
}

void MainWindow::onShowPrimaryMenu(const Glib::VariantBase& value)
{
  menu_button_->activate();
}

void MainWindow::onShowProfiles(const Glib::VariantBase& value)
{
  profiles_button_->activate();
}

void MainWindow::onShowPreferences(const Glib::VariantBase& parameter)
{
  preferences_dialog_->present();
}

void MainWindow::onShowShortcuts(const Glib::VariantBase& parameter)
{
  static const Glib::ustring ui_header =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<interface>\n"
    "  <object class=\"GtkShortcutsWindow\" id=\"shortcutsWindow\">\n"
    "    <property name=\"modal\">1</property>\n";

  static const Glib::ustring ui_footer =
    "  </object>\n"
    "</interface>\n";

  static const Glib::ustring ui_section_header =
    "    <child>\n"
    "      <object class=\"GtkShortcutsSection\">\n"
    "        <property name=\"visible\">1</property>\n"
    "        <property name=\"section-name\">shortcuts</property>\n"
    "        <property name=\"max-height\">10</property>\n";

  static const Glib::ustring ui_section_footer =
    "      </object>\n"
    "    </child>\n";


  static const Glib::ustring ui_group_header =
    "        <child>\n"
    "          <object class=\"GtkShortcutsGroup\">\n"
    "            <property name=\"visible\">1</property>\n"
    "            <property name=\"title\">%1</property>\n";

  static const Glib::ustring ui_group_footer =
    "          </object>\n"
    "        </child>\n";

  static const Glib::ustring ui_shortcut_header =
    "            <child>\n"
    "              <object class=\"GtkShortcutsShortcut\">\n"
    "                <property name=\"visible\">1</property>\n"
    "                <property name=\"accelerator\">%1</property>\n"
    "                <property name=\"title\">%2</property>\n";

  static const Glib::ustring ui_shortcut_footer =
    "              </object>\n"
    "            </child>\n";

  Glib::ustring ui = ui_header + ui_section_header;

  Glib::ustring group_title;
  bool group_open = false;

  for (const auto& [key, title] : ShortcutList())
  {
    if (key.empty())
    {
      if (group_open)
      {
        ui += ui_group_footer;
        group_open = false;
      }
      group_title = title;
    }
    else
    {
      auto accel = settings_shortcuts_->get_string(key);

        // validate accelerator
      guint accel_key;
      GdkModifierType accel_mods;

      gtk_accelerator_parse(accel.c_str(), &accel_key, &accel_mods);

      if (accel_key != 0 || accel_mods != 0)
      {
        if (!group_open)
        {
          ui += Glib::ustring::compose(ui_group_header,
                                       Glib::Markup::escape_text(group_title));
          group_open = true;
        }
        ui += Glib::ustring::compose(ui_shortcut_header,
                                     Glib::Markup::escape_text(accel),
                                     Glib::Markup::escape_text(title));
        ui += ui_shortcut_footer;
      }
    }
  }

  if (group_open)
    ui += ui_group_footer;

  ui += ui_section_footer;
  ui += ui_footer;

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_string(ui);

  if (shortcuts_window_ != nullptr)
  {
    delete shortcuts_window_;
    shortcuts_window_ = nullptr;
  }

  builder->get_widget("shortcutsWindow", shortcuts_window_);

  shortcuts_window_->unset_view_name();
  shortcuts_window_->property_section_name() = "shortcuts";
  shortcuts_window_->present();
}

void MainWindow::onShowHelp(const Glib::VariantBase& parameter)
{}

void MainWindow::onShowAbout(const Glib::VariantBase& parameter)
{
  about_dialog_.present();
}

void MainWindow::onToggleFullScreen(const Glib::VariantBase& parameter)
{
  auto new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(parameter);

  if (new_state.get())
    fullscreen();
  else
    unfullscreen();
}

void MainWindow::onMeterChanged()
{
  Glib::ustring param_str = meter_combo_box_->get_active_id();

  Gtk::Application::get_default()
    ->activate_action(kActionMeterSelect, Glib::Variant<Glib::ustring>::create(param_str));
}

void MainWindow::onBeatsChanged()
{
  auto app = Gtk::Application::get_default();

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();

  Meter meter;
  app->get_action_state(meter_slot, meter);

  int beats = std::lround(beats_adjustment_->get_value());

  meter.setBeats(beats);

  auto new_state = Glib::Variant<Meter>::create(meter);
  app->activate_action(meter_slot, new_state);
}

void MainWindow::onSubdivChanged()
{
  auto app = Gtk::Application::get_default();

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();

  Meter meter;
  app->get_action_state(meter_slot, meter);

  int division = std::atoi(subdiv_combo_box_->get_active_id().c_str());

  meter.setDivision(division);

  auto new_state = Glib::Variant<Meter>::create(meter);
  app->activate_action(meter_slot, new_state);
}

void MainWindow::onAccentChanged(std::size_t button_index)
{
  std::size_t beats = std::lround(beats_adjustment_->get_value());
  std::size_t division = std::atoi(subdiv_combo_box_->get_active_id().c_str());
  std::size_t pattern_size = std::min(beats * division, accent_button_grid_.size());

  AccentPattern pattern(pattern_size);

  for (std::size_t index = 0; index < pattern_size; ++index)
  {
    pattern[index] = accent_button_grid_[index].getAccentState();
  }

  Meter meter(beats, division, pattern);

  auto state = Glib::Variant<Meter>::create(meter);

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();
  Gtk::Application::get_default()->activate_action(meter_slot, state);
}

void MainWindow::onProfilesSelectionChanged()
{
  Glib::ustring id;

  auto row_it = profiles_tree_view_->get_selection()->get_selected();

  if (row_it)
    id = row_it->get_value(profiles_list_store_->columns_.id_);

  auto state = Glib::Variant<Glib::ustring>::create(id);

  Gtk::Application::get_default()->activate_action(kActionProfilesSelect, state);
}

void MainWindow::onProfilesTitleStartEditing(Gtk::CellEditable* editable,
                                             const Glib::ustring& path_string)
{
  Gtk::TreePath path(path_string);
  ProfilesListStore::iterator row_it = profiles_list_store_->get_iter(path);

  auto tree_selection = profiles_tree_view_->get_selection();

  // do not edit titles of unselected profiles
  if (row_it != tree_selection->get_selected())
    editable->editing_done();
}

void MainWindow::onProfilesTitleChanged(const Glib::ustring& path_string,
                                        const Glib::ustring& text)
{
  auto app = Gtk::Application::get_default();

  Gtk::TreePath path(path_string);

  ProfilesListStore::iterator row_it = profiles_list_store_->get_iter(path);
  if(row_it)
  {
    Glib::ustring selected_id;
    app->get_action_state(kActionProfilesSelect, selected_id);

    if (selected_id == (*row_it)[profiles_list_store_->columns_.id_])
    {
      auto state = Glib::Variant<Glib::ustring>::create(text);
      app->activate_action(kActionProfilesTitle, state);
    }

    profiles_tree_view_->get_selection()->select(path);
  }
}

void MainWindow::onProfilesDragBegin(const Glib::RefPtr<Gdk::DragContext>& context)
{
  profiles_selection_changed_connection_.block();
}

void MainWindow::onProfilesDragEnd(const Glib::RefPtr<Gdk::DragContext>& context)
{
  profiles_selection_changed_connection_.unblock();

  auto rows = profiles_list_store_->children();
  ProfilesIdentifierList id_list;
  for (const auto& row :rows)
    {
      id_list.push_back(row[profiles_list_store_->columns_.id_]);
    }
  Gtk::Application::get_default()
    ->activate_action(kActionProfilesReorder, Glib::Variant<ProfilesIdentifierList>::create(id_list));

  auto app = Gtk::Application::get_default();
  Glib::ustring id;
  app->get_action_state(kActionProfilesSelect, id);

  updateProfilesSelect(id);
}

void MainWindow::onActionStateChanged(const Glib::ustring& action_name,
                                      const Glib::VariantBase& variant)
{
  auto app = Gtk::Application::get_default();

  if (action_name.compare(0,6,"meter-") == 0)
  {
    Glib::ustring meter_slot;
    app->get_action_state(kActionMeterSelect, meter_slot);

    if (action_name == kActionMeterSelect || action_name == meter_slot)
    {
      Meter meter;
      app->get_action_state(meter_slot, meter);

      updateMeterInterface(meter_slot, meter);
    }
  }
  else if (action_name.compare(kActionTempo) == 0)
  {
    double tempo;
    app->get_action_state(kActionTempo, tempo);

    updateTempo(tempo);
  }
  else if (action_name.compare(kActionStart) == 0)
  {
    bool running;
    app->get_action_state(kActionStart, running);

    updateStart(running);
  }
  else if (action_name.compare(kActionProfilesList) == 0)
  {
    ProfilesList list;
    app->get_action_state(kActionProfilesList, list);

    updateProfilesList(list);
  }
  else if (action_name.compare(kActionProfilesSelect) == 0)
  {
    Glib::ustring id;
    app->get_action_state(kActionProfilesSelect, id);

    updateProfilesSelect(id);
  }
  else if (action_name.compare(kActionProfilesTitle) == 0)
  {
    Glib::ustring title;
    app->get_action_state(kActionProfilesTitle, title);

    updateProfilesTitle(title);
  }
}

void MainWindow::updateMeterInterface(const Glib::ustring& slot, const Meter& meter)
{
  std::for_each(meter_connections_.begin(), meter_connections_.end(),
                std::bind(&sigc::connection::block, std::placeholders::_1, true));

  Glib::ustring active_id = meter_combo_box_->get_active_id();
  if (active_id != slot)
    meter_combo_box_->set_active_id(slot);

  if (slot == kActionMeterCustom)
  {
    beats_label_->set_sensitive(true);
    beats_spin_button_->set_sensitive(true);
    subdiv_label_->set_sensitive(true);
    subdiv_combo_box_->set_sensitive(true);
  }
  else {
    beats_label_->set_sensitive(false);
    beats_spin_button_->set_sensitive(false);
    subdiv_label_->set_sensitive(false);
    subdiv_combo_box_->set_sensitive(false);
  }

  beats_adjustment_->set_value(meter.beats());

  subdiv_combo_box_->set_active_id(Glib::ustring::format(meter.division()));

  updateAccentButtons(meter);

  std::for_each(meter_connections_.begin(), meter_connections_.end(),
                std::mem_fn(&sigc::connection::unblock));
}

void MainWindow::updateAccentButtons(const Meter& meter)
{
  std::size_t new_grouping = meter.division();
  std::size_t new_size = meter.beats() * new_grouping;
  auto& new_accents = meter.accents();

  bool need_relabel = false;

  if (new_size > accent_button_grid_.size()
      || new_grouping != accent_button_grid_.grouping())
  {
    need_relabel = true;
  }

  accent_button_grid_.resize(new_size);
  accent_button_grid_.regroup(meter.division());

  if (need_relabel)
  {
    for (std::size_t index = 0; index < accent_button_grid_.size(); ++index)
    {
      Glib::ustring label = ( index % new_grouping == 0 ) ?
        Glib::ustring::format( index / new_grouping + 1 ) : "";

      accent_button_grid_[index].setLabel(label);
      accent_button_grid_[index].setAccentState( new_accents[index] );
    }
  }
  else
  {
    for (std::size_t index = 0; index < accent_button_grid_.size(); ++index)
    {
      accent_button_grid_[index].setAccentState( new_accents[index] );
    }
  }
}

void MainWindow::updateProfilesList(const ProfilesList& list)
{
  profiles_selection_changed_connection_.block();

  profiles_list_store_->clear();

  for (const ProfilesListEntry& e : list)
  {
    ProfilesListStore::Row row = *(profiles_list_store_->append());

    row[profiles_list_store_->columns_.id_]
      = std::get<kProfilesListEntryIdentifier>(e);

    row[profiles_list_store_->columns_.title_]
      = std::get<kProfilesListEntryTitle>(e);

    row[profiles_list_store_->columns_.description_]
      = std::get<kProfilesListEntryDescription>(e);
  }

  profiles_selection_changed_connection_.unblock();
}

void MainWindow::updateProfilesSelect(const Glib::ustring& id)
{
  auto rows = profiles_list_store_->children();

  auto it = std::find_if(rows.begin(), rows.end(),
                         [this,&id] (const auto& row) -> bool {
                           return row[profiles_list_store_->columns_.id_] == id;
                         });

  profiles_selection_changed_connection_.block();
  {
    if (it!=rows.end())
      profiles_tree_view_->get_selection()->select(it);
    else
      profiles_tree_view_->get_selection()->unselect_all();
  }
  profiles_selection_changed_connection_.unblock();
}

void MainWindow::updateProfilesTitle(const Glib::ustring& title)
{
  if (title.empty())
  {
    current_profile_label_->hide();
    current_profile_label_->set_text(title);
    Gtk::Window::set_title(Glib::get_application_name());
  }
  else
  {
    current_profile_label_->show();
    current_profile_label_->set_text(title);
    Gtk::Window::set_title(Glib::get_application_name() + " - " + title);
  }
}

void MainWindow::updateTempo(double tempo)
{
  cancelButtonAnimations();
}

void MainWindow::updateStart(bool running)
{
  if (!running)
    cancelButtonAnimations();
}

void MainWindow::cancelButtonAnimations()
{
  for (auto button : accent_button_grid_.buttons())
    button->cancelAnimation();
}

void MainWindow::updateAccentAnimation(const audio::Ticker::Statistics& stats)
{
  std::size_t next_accent = stats.generator.next_accent;

  uint64_t time = stats.timestamp.count()
    + stats.backend_latency.count()
    + stats.generator.next_accent_delay.count();

  time += animation_sync_usecs_;

  if ( next_accent < accent_button_grid_.size() )
  {
    switch (meter_animation_)
    {
    case settings::kMeterAnimationBeat:
      if (next_accent % accent_button_grid_.grouping() != 0)
        break;
      [[fallthrough]];

    case settings::kMeterAnimationAll:
      accent_button_grid_[next_accent].scheduleAnimation(time);
      break;

    default:
      break;
    };
  }
}

void MainWindow::updateCurrentTempo(const audio::Ticker::Statistics& stats)
{
  static const Glib::ustring kAccelUpSymbol     = (u8"▴");
  static const Glib::ustring kAccelDownSymbol   = (u8"▾");
  static const Glib::ustring kAccelStableSymbol = (u8"•");

  static const int precision = 2;

  double tempo_integral;
  double tempo_fraction;

  tempo_fraction = std::modf(stats.generator.current_tempo, &tempo_integral);

  int tempo_integral_int = tempo_integral;
  int tempo_fraction_int = tempo_fraction * std::pow(10, precision);

  auto text = Glib::ustring::format(tempo_integral_int);

  if (text != tempo_integral_label_->get_text())
    tempo_integral_label_->set_text(text);

  text = Glib::ustring::format(std::setfill(L'0'), std::setw(precision), tempo_fraction_int);

  if (text != tempo_fraction_label_->get_text())
    tempo_fraction_label_->set_text(text);

  if (stats.generator.current_accel == 0)
     text = kAccelStableSymbol;
  else if (stats.generator.current_accel > 0)
     text = kAccelUpSymbol;
  else if (stats.generator.current_accel < 0)
    text = kAccelDownSymbol;

  if (text != tempo_divider_label_->get_text())
    tempo_divider_label_->set_text(text);
}

void MainWindow::onTickerStatistics(const audio::Ticker::Statistics& stats)
{
  updateCurrentTempo(stats);

  if (meter_animation_ != settings::kMeterAnimationOff)
    updateAccentAnimation(stats);
}

void MainWindow::onMessage(const Message& message)
{
  info_topic_label_->set_text(message.topic);
  info_text_label_->set_markup(message.text);
  info_details_label_->set_text(message.details);

  switch ( message.category )
  {
  case MessageCategory::kInformation:
    info_bar_->set_message_type(Gtk::MESSAGE_INFO);
    info_image_->set_from_icon_name("dialog-information", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    break;
  case MessageCategory::kError:
    info_bar_->set_message_type(Gtk::MESSAGE_ERROR);
    info_image_->set_from_icon_name("dialog-error", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    break;
  case MessageCategory::kWarning:
    [[fallthrough]];
  default:
    info_bar_->set_message_type(Gtk::MESSAGE_WARNING);
    info_image_->set_from_icon_name("dialog-warning", Gtk::ICON_SIZE_LARGE_TOOLBAR);
  }

  if ( message.details.empty() )
    info_details_expander_->hide();
  else
    info_details_expander_->show();

  info_details_expander_->set_expanded(false);
  info_revealer_->set_reveal_child(true);
}

void MainWindow::onMessageResponse(int response)
{
  if (response == Gtk::RESPONSE_CLOSE)
  {
    info_revealer_->set_reveal_child(false);
  }
}

void MainWindow::onSettingsPrefsChanged(const Glib::ustring& key)
{
  if (key == settings::kKeyPrefsAnimationSync)
  {
    updatePrefAnimationSync();
  }
  else if (key == settings::kKeyPrefsMeterAnimation)
  {
    updatePrefMeterAnimation();
  }
}

void MainWindow::updatePrefMeterAnimation()
{
  meter_animation_ = settings_prefs_->get_enum(settings::kKeyPrefsMeterAnimation);
}

void MainWindow::updatePrefAnimationSync()
{
  animation_sync_usecs_ =
    std::round( settings_prefs_->get_double(settings::kKeyPrefsAnimationSync) * 1000.);
}
