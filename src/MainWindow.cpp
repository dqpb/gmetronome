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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "MainWindow.h"
#include "Application.h"
#include "ActionBinding.h"
#include "ProfileListStore.h"
#include "SettingsDialog.h"
#include "AccentButton.h"
#include "Settings.h"
#include "Shortcut.h"

#include <glibmm/i18n.h>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <algorithm>
#include <cmath>

namespace {

  void onCssParsingError(const Glib::RefPtr<const Gtk::CssSection>& section,
                         const Glib::Error& error)
  {
    std::cerr << "CSS parsing error: " << error.what() << std::endl;
    if (section)
    {
      if (const auto file = section->get_file(); file)
        std::cerr << "  URI = " << file->get_uri() << std::endl;

      std::cerr << "  start_line = " << section->get_start_line()+1
                << ", end_line = " << section->get_end_line()+1 << std::endl;
      std::cerr << "  start_position = " << section->get_start_position()
                << ", end_position = " << section->get_end_position() << std::endl;
    }
  }

  Glib::RefPtr<Gtk::CssProvider> getGlobalCssProvider()
  {
    static Glib::RefPtr<Gtk::CssProvider> css_provider;

    if (!css_provider)
    {
      css_provider = Gtk::CssProvider::create();
      css_provider->signal_parsing_error().connect(&onCssParsingError);

      Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(),
        css_provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      try
      {
        auto css_resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/css/global.css";
        css_provider->load_from_resource(css_resource_path);
      }
      catch(const Gtk::CssProviderError& ex)
      {
        std::cerr << "Global CSS: CssProviderError, Gtk::CssProvider::load_from_path() failed: "
                  << ex.what() << std::endl;
      }
      catch(const Glib::Error& ex)
      {
        std::cerr << "Global CSS: Gtk::CssProvider::load_from_path() failed: "
                  << ex.what() << std::endl;
      }
    }
    return css_provider;
  }

  Glib::RefPtr<Gtk::CssProvider> registerGlobalCssProvider() {
    return getGlobalCssProvider();
  }

}//unnamed namespace

//static
MainWindow* MainWindow::create()
{
  auto icons_resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/icons";
  Gtk::IconTheme::get_default()->add_resource_path(icons_resource_path);

  // Load the Builder file and instantiate its widgets.
  auto win_resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/ui/MainWindow.glade";
  auto builder_ = Gtk::Builder::create_from_resource(win_resource_path);

  MainWindow* window = nullptr;
  builder_->get_widget_derived("mainWindow", window);
  if (!window)
    throw std::runtime_error("No \"mainWindow\" object in MainWindow.glade");

  return window;
}

MainWindow::MainWindow(BaseObjectType* cobject,
                       const Glib::RefPtr<Gtk::Builder>& builder)
  : Gtk::ApplicationWindow(cobject),
    builder_{builder},
    shortcuts_window_{nullptr},
    animation_sync_{0},
    bottom_resizable_{true}
{
  // install global css provider for default screen
  registerGlobalCssProvider();

  builder_->get_widget("headerBar", header_bar_);
  builder_->get_widget("tempoIntegralLabel", tempo_integral_label_);
  builder_->get_widget("tempoFractionLabel", tempo_fraction_label_);
  builder_->get_widget("tempoDividerLabel", tempo_divider_label_);
  builder_->get_widget("currentProfileLabel", current_profile_label_);
  builder_->get_widget("fullScreenButton", full_screen_button_);
  builder_->get_widget("fullScreenImage", full_screen_image_);
  builder_->get_widget("mainMenuButton", main_menu_button_);
  builder_->get_widget("popoverMenu", popover_menu_);
  builder_->get_widget("profileMenuButton", profile_menu_button_);
  builder_->get_widget("profilePopover", profile_popover_);
  builder_->get_widget("profileTreeView", profile_tree_view_);
  builder_->get_widget("profileNewButton", profile_new_button_);
  builder_->get_widget("profileDeleteButton", profile_delete_button_);
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
  builder_->get_widget("trainerToggleButton", trainer_toggle_button_);
  builder_->get_widget("accentToggleButton", accent_toggle_button_);
  builder_->get_widget("trainerRevealer", trainer_revealer_);
  builder_->get_widget("accentRevealer", accent_revealer_);
  builder_->get_widget("pendulumRevealer", pendulum_revealer_);
  builder_->get_widget("pendulumBox", pendulum_box_);
  builder_->get_widget("trainerFrame", trainer_frame_);
  builder_->get_widget("accentFrame", accent_frame_);
  builder_->get_widget("accentBox", accent_box_);
  builder_->get_widget("tempoScale", tempo_scale_);
  builder_->get_widget("tapEventBox", tap_event_box_);
  builder_->get_widget("tapLevelBar", tap_level_bar_);
  builder_->get_widget("meterComboBox", meter_combo_box_);
  builder_->get_widget("beatsSpinButton", beats_spin_button_);
  builder_->get_widget("beatsLabel", beats_label_);
  builder_->get_widget("subdivButtonBox", subdiv_button_box_);
  builder_->get_widget("subdiv1RadioButton", subdiv_1_radio_button_);
  builder_->get_widget("subdiv2RadioButton", subdiv_2_radio_button_);
  builder_->get_widget("subdiv3RadioButton", subdiv_3_radio_button_);
  builder_->get_widget("subdiv4RadioButton", subdiv_4_radio_button_);
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

  profile_title_new_ = gettext(Profile::kDefaultTitle.c_str());
  profile_title_placeholder_ = gettext(Profile::kDefaultTitlePlaceholder.c_str());

  profile_list_store_ = ProfileListStore::create();
  profile_tree_view_->set_model(profile_list_store_);
  profile_tree_view_->append_column_editable("Title", profile_list_store_->columns_.title_);
  profile_tree_view_->enable_model_drag_source();
  profile_tree_view_->enable_model_drag_dest();

  preferences_dialog_ = SettingsDialog::create(*this);

  initActions();
  initUI();
  initBindings();

  updatePrefPendulumAction();
  updatePrefPendulumPhaseMode();
  updatePrefMeterAnimation();
  updatePrefAnimationSync();
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
      {kActionShowPendulum,            settings::state()},
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
  updateCurrentTempo( { 0us, 0.0, 0.0, -1.0, -1, 0us, 0us } );

  // initialize info bar
  info_overlay_->add_overlay(*info_revealer_);
  info_revealer_->set_reveal_child(false);

  // initialize pendulum
  pendulum_box_->pack_start(pendulum_, Gtk::PACK_EXPAND_WIDGET);
  pendulum_.set_halign(Gtk::ALIGN_CENTER);
  pendulum_.show();

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
  int id_col = meter_combo_box_->get_id_column();
  meter_combo_box_->set_row_separator_func( [id_col] (const Glib::RefPtr<Gtk::TreeModel>& model,
                                                      const Gtk::TreeModel::iterator& iter) -> bool
    {
      Gtk::TreeModel::Row row = *iter;
      Glib::ustring id_str;
      row.get_value(id_col, id_str);
      return id_str == "separator";
    });

  Glib::ustring meter_slot;
  app->get_action_state(kActionMeterSelect, meter_slot);

  Meter meter;
  app->get_action_state(meter_slot, meter);
  updateMeter(meter_slot, meter);

  // initialize profile list
  ProfileList list;
  app->get_action_state(kActionProfileList, list);
  updateProfileList(list);

  // initalize profile selection
  Glib::ustring id;
  app->get_action_state(kActionProfileSelect, id);
  updateProfileSelect(id);

  // initialize profile title
  Glib::ustring title;
  app->get_action_state(kActionProfileTitle, title);
  updateProfileTitle(title, !id.empty());
}

void MainWindow::initBindings()
{
  auto app = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  settings::preferences()->signal_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onSettingsPrefsChanged));

  settings::sound()->bind(settings::kKeySoundVolume,
                          volume_button_,
                          "value",
                          Gio::SETTINGS_BIND_DEFAULT);

  settings::state()->bind(settings::kKeyStateShowPendulum,
                          pendulum_revealer_,
                          "reveal-child",
                          Gio::SETTINGS_BIND_GET);

  settings::state()->bind(settings::kKeyStateShowPendulum,
                          pendulum_revealer_,
                          "vexpand",
                          Gio::SETTINGS_BIND_GET);
  bindings_
    .push_back( Glib::Binding::bind_property( trainer_toggle_button_->property_active(),
                                              trainer_frame_->property_sensitive() ));
  bindings_
    .push_back( Glib::Binding::bind_property( trainer_toggle_button_->property_active(),
                                              trainer_revealer_->property_reveal_child() ));
  bindings_
    .push_back( Glib::Binding::bind_property( accent_toggle_button_->property_active(),
                                              accent_frame_->property_sensitive() ));
  bindings_
    .push_back( Glib::Binding::bind_property( accent_toggle_button_->property_active(),
                                              accent_revealer_->property_reveal_child() ));

  tap_event_box_->add_events(Gdk::BUTTON_PRESS_MASK);
  tap_event_box_->signal_button_press_event()
    .connect(sigc::mem_fun(*this, &MainWindow::onTempoTap));

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
    .push_back( subdiv_1_radio_button_->signal_clicked()
                .connect([&]{onSubdivChanged(subdiv_1_radio_button_, 1);})
      );
  meter_connections_
    .push_back( subdiv_2_radio_button_->signal_clicked()
                .connect([&]{onSubdivChanged(subdiv_2_radio_button_, 2);})
      );
  meter_connections_
    .push_back( subdiv_3_radio_button_->signal_clicked()
                .connect([&]{onSubdivChanged(subdiv_3_radio_button_, 3);})
      );
  meter_connections_
    .push_back( subdiv_4_radio_button_->signal_clicked()
                .connect([&]{onSubdivChanged(subdiv_4_radio_button_, 4);})
      );
  meter_connections_
    .push_back( accent_button_grid_.signal_accent_changed()
                .connect(sigc::mem_fun(*this, &MainWindow::onAccentChanged))
      );

  profile_tree_view_->signal_drag_begin()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileDragBegin));

  profile_tree_view_->signal_drag_end()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileDragEnd));

  profile_selection_changed_connection_ =
    profile_tree_view_->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileSelectionChanged));

  auto cell_renderer = dynamic_cast<Gtk::CellRendererText*>(
    profile_tree_view_->get_column_cell_renderer(0));

  cell_renderer->property_placeholder_text() = profile_title_placeholder_;

    cell_renderer->signal_editing_started()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileTitleStartEditing));

    cell_renderer->signal_edited()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileTitleChanged));

  profile_new_button_->signal_clicked()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileNew));

  app->signal_action_state_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onActionStateChanged));

  profile_popover_->signal_show()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileShow));

  profile_popover_->signal_hide()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileHide));

  app->signalMessage()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessage));

  info_bar_->signal_response()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessageResponse));

  app->signalTickerStatistics()
    .connect(sigc::mem_fun(*this, &MainWindow::onTickerStatistics));

  app->signalTap()
    .connect(sigc::mem_fun(*this, &MainWindow::onTap));

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

  auto win_state = window_state_event->new_window_state;

  bool fullscreen = win_state & GDK_WINDOW_STATE_FULLSCREEN;
  bool maximized = win_state & GDK_WINDOW_STATE_MAXIMIZED;
  [[maybe_unused]]
  bool bottom_resizable = win_state & GDK_WINDOW_STATE_BOTTOM_RESIZABLE; // never set?
  bool tiled = win_state & GDK_WINDOW_STATE_TILED; // deprecated

  if (fullscreen || maximized || tiled) // || !bottom_resizable
    bottom_resizable_ = false;
  else
    bottom_resizable_ = true;

  if (fullscreen)
  {
    header_bar_->reparent(*main_box_);
    header_bar_->set_decoration_layout(":minimize,close");
    main_box_->reorder_child(*header_bar_, 0);
    full_screen_image_->set_from_icon_name("view-restore-symbolic",
                                           Gtk::ICON_SIZE_BUTTON);
    full_screen_button_->show();
  }
  else
  {
    header_bar_->reparent(titlebar_bin_);
    header_bar_->unset_decoration_layout();
    full_screen_image_->set_from_icon_name("view-fullscreen-symbolic",
                                           Gtk::ICON_SIZE_BUTTON);
    full_screen_button_->hide();
  }

  Glib::RefPtr<Gio::Action> action = lookup_action(kActionFullScreen);
  auto simple_action =  Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
  simple_action->set_state(Glib::Variant<bool>::create(fullscreen));

  return true;
}

bool MainWindow::onTempoTap(GdkEventButton* button_event)
{
  if (button_event->type != GDK_2BUTTON_PRESS
      && button_event->type != GDK_3BUTTON_PRESS)
  {
    Gtk::Application::get_default()->activate_action(kActionTempoTap);
  }
  return true;
}

void MainWindow::onTempoLabelAllocate(Gtk::Allocation& alloc)
{
  // make sure that the header bar is always high enough
  // for the tempo label and the profile name label
  header_bar_->set_size_request(-1, 2 * alloc.get_height() + 10);
}

void MainWindow::onProfileShow()
{
  profile_tree_view_->set_can_focus(true);

  if (profile_tree_view_->get_selection()->count_selected_rows() != 0)
    profile_tree_view_->property_has_focus() = true;
  else
    profile_new_button_->property_has_focus() = true;
}

void MainWindow::onProfileHide()
{
  profile_tree_view_->set_can_focus(false);
}

void MainWindow::onShowPrimaryMenu(const Glib::VariantBase& value)
{
  main_menu_button_->activate();
}

void MainWindow::onShowProfiles(const Glib::VariantBase& value)
{
  profile_menu_button_->activate();
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
      auto accel = settings::shortcuts()->get_string(key);

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

void MainWindow::activateMeterAction(const Glib::ustring& action,
                                     const Glib::VariantBase& param)
{
  auto app = Gtk::Application::get_default();

  last_meter_action_ = g_get_monotonic_time();

  if (pendulum_restore_connection_.empty()
      && pendulum_revealer_->get_child_revealed()
      && bottom_resizable_)
  {
    pendulum_revealer_->set_size_request(
      pendulum_revealer_->get_width(),
      pendulum_revealer_->get_height()
      );
    pendulum_revealer_->set_vexpand(false);

    pendulum_restore_connection_ = Glib::signal_timeout().connect(
      [this] () -> bool {
        if (g_get_monotonic_time() - last_meter_action_ > 100000)
        {
          pendulum_revealer_->set_size_request(-1,-1);
          pendulum_revealer_->set_vexpand(true);
          return false;
        }
        else return true;
      }, 100);
  }

  app->activate_action(action, param);

  if (pendulum_revealer_->get_child_revealed() && bottom_resizable_)
  {
    int win_width, win_height;
    get_size(win_width, win_height);
    resize(win_width, 1);
  }
}

void MainWindow::onMeterChanged()
{
  Glib::ustring param_str = meter_combo_box_->get_active_id();
  activateMeterAction(kActionMeterSelect, Glib::Variant<Glib::ustring>::create(param_str));
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
  activateMeterAction(meter_slot, new_state);
}

void MainWindow::onSubdivChanged(Gtk::RadioButton* button, int division)
{
  if (!button->get_active())
    return;

  auto app = Gtk::Application::get_default();

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();

  Meter meter;
  app->get_action_state(meter_slot, meter);

  meter.setDivision(division);

  auto new_state = Glib::Variant<Meter>::create(meter);
  activateMeterAction(meter_slot, new_state);
}

void MainWindow::onAccentChanged(std::size_t button_index)
{
  std::size_t beats = std::lround(beats_adjustment_->get_value());

  std::size_t division = 1;
  if (subdiv_2_radio_button_->get_active())
    division = 2;
  else if (subdiv_3_radio_button_->get_active())
    division = 3;
  else if (subdiv_4_radio_button_->get_active())
    division = 4;

  std::size_t pattern_size = std::min(beats * division, accent_button_grid_.size());

  AccentPattern pattern(pattern_size);

  for (std::size_t index = 0; index < pattern_size; ++index)
  {
    pattern[index] = accent_button_grid_[index].getAccentState();
  }

  Meter meter(division, beats, pattern);

  auto state = Glib::Variant<Meter>::create(meter);

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();
  Gtk::Application::get_default()->activate_action(meter_slot, state);
}

void MainWindow::onProfileSelectionChanged()
{
  Glib::ustring id;

  auto row_it = profile_tree_view_->get_selection()->get_selected();

  if (row_it)
    id = row_it->get_value(profile_list_store_->columns_.id_);

  auto state = Glib::Variant<Glib::ustring>::create(id);

  Gtk::Application::get_default()->activate_action(kActionProfileSelect, state);
}

void MainWindow::onProfileTitleStartEditing(Gtk::CellEditable* editable,
                                            const Glib::ustring& path_string)
{
  Gtk::TreePath path(path_string);
  ProfileListStore::iterator row_it = profile_list_store_->get_iter(path);

  auto tree_selection = profile_tree_view_->get_selection();

  // do not edit titles of unselected profiles
  if (row_it != tree_selection->get_selected())
    editable->editing_done();
}

void MainWindow::onProfileTitleChanged(const Glib::ustring& path_string,
                                       const Glib::ustring& text)
{
  auto app = Gtk::Application::get_default();

  Gtk::TreePath path(path_string);

  ProfileListStore::iterator row_it = profile_list_store_->get_iter(path);
  if(row_it)
  {
    Glib::ustring selected_id;
    app->get_action_state(kActionProfileSelect, selected_id);

    if (selected_id == (*row_it)[profile_list_store_->columns_.id_])
    {
      auto state = Glib::Variant<Glib::ustring>::create(text);
      app->activate_action(kActionProfileTitle, state);
    }

    profile_tree_view_->get_selection()->select(path);
  }
}

void MainWindow::onProfileDragBegin(const Glib::RefPtr<Gdk::DragContext>& context)
{
  profile_selection_changed_connection_.block();
}

void MainWindow::onProfileDragEnd(const Glib::RefPtr<Gdk::DragContext>& context)
{
  profile_selection_changed_connection_.unblock();

  auto rows = profile_list_store_->children();

  ProfileIdentifierList id_list;
  id_list.reserve(rows.size());

  std::transform(rows.begin(), rows.end(), std::back_inserter(id_list),
                 [this] (const auto& row) {
                   return row[profile_list_store_->columns_.id_];
                 });

  Gtk::Application::get_default()
    ->activate_action(kActionProfileReorder, Glib::Variant<ProfileIdentifierList>::create(id_list));

  auto app = Gtk::Application::get_default();
  Glib::ustring id;
  app->get_action_state(kActionProfileSelect, id);

  updateProfileSelect(id);
}

void MainWindow::onProfileNew()
{
  static const auto title = Glib::Variant<Glib::ustring>::create(profile_title_new_);
  Gtk::Application::get_default()
    ->activate_action(kActionProfileNew, title);
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

      updateMeter(meter_slot, meter);
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
  else if (action_name.compare(kActionProfileList) == 0)
  {
    ProfileList list;
    app->get_action_state(kActionProfileList, list);
    updateProfileList(list);

    Glib::ustring id;
    app->get_action_state(kActionProfileSelect, id);
    updateProfileSelect(id);
  }
  else if (action_name.compare(kActionProfileSelect) == 0)
  {
    Glib::ustring id;
    app->get_action_state(kActionProfileSelect, id);
    updateProfileSelect(id);

    // switching from a profile-less state to an untitled profile does not
    // change the state of kActionProfileTitle, but requires to update
    // the title nevertheless
    Glib::ustring title;
    app->get_action_state(kActionProfileTitle, title);

    if (title.empty())
      updateProfileTitle(title, !id.empty());
  }
  else if (action_name.compare(kActionProfileTitle) == 0)
  {
    Glib::ustring id;
    app->get_action_state(kActionProfileSelect, id);

    Glib::ustring title;
    app->get_action_state(kActionProfileTitle, title);

    updateProfileTitle(title, !id.empty());
  }
}

void MainWindow::updateMeter(const Glib::ustring& slot, const Meter& meter)
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
    subdiv_button_box_->set_sensitive(true);
  }
  else {
    beats_label_->set_sensitive(false);
    beats_spin_button_->set_sensitive(false);
    subdiv_label_->set_sensitive(false);
    subdiv_button_box_->set_sensitive(false);
  }

  beats_adjustment_->set_value(meter.beats());

  switch (meter.division())
  {
  case 1: subdiv_1_radio_button_->set_active(); break;
  case 2: subdiv_2_radio_button_->set_active(); break;
  case 3: subdiv_3_radio_button_->set_active(); break;
  case 4: subdiv_4_radio_button_->set_active(); break;
  default:
    // do nothing
    break;
  };

  updateAccentButtons(meter);

  std::for_each(meter_connections_.begin(), meter_connections_.end(),
                std::mem_fn(&sigc::connection::unblock));

  pendulum_.setMeter(meter);
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

void MainWindow::updateProfileList(const ProfileList& list)
{
  auto& col_id    = profile_list_store_->columns_.id_;
  auto& col_title = profile_list_store_->columns_.title_;
  auto& col_descr = profile_list_store_->columns_.description_;

  profile_selection_changed_connection_.block();

  auto children = profile_list_store_->children();
  auto rowit = children.begin();

  for (const auto& [id, title, descr] : list)
  {
    if (rowit == children.end() || rowit->get_value(col_id) != id)
    {
      auto tmp_rowit = std::find_if(rowit, children.end(),
                                    [&col_id,&id = id] (const auto& row) {
                                      return row[col_id] == id;
                                    });

      if (tmp_rowit != children.end())
      {
        profile_list_store_->move(tmp_rowit, rowit);
        rowit = tmp_rowit;
      }
      else
      {
        rowit = profile_list_store_->insert(rowit);
      }
    }
    // update row
    rowit->set_value(col_id, id);
    rowit->set_value(col_title, title);
    rowit->set_value(col_descr, descr);
    ++rowit;
  }

  while (rowit != children.end())
    rowit = profile_list_store_->erase(rowit);

  profile_selection_changed_connection_.unblock();
}

void MainWindow::updateProfileSelect(const Glib::ustring& id)
{
  auto rows = profile_list_store_->children();

  auto it = std::find_if(rows.begin(), rows.end(),
                         [this,&id] (const auto& row) -> bool {
                           return row[profile_list_store_->columns_.id_] == id;
                         });

  profile_selection_changed_connection_.block();
  {
    if (it!=rows.end())
      profile_tree_view_->get_selection()->select(it);
    else
      profile_tree_view_->get_selection()->unselect_all();
  }
  profile_selection_changed_connection_.unblock();
}

void MainWindow::updateProfileTitle(const Glib::ustring& title, bool has_profile)
{
  if (has_profile)
  {
    static const double default_opacity = 0.7;
    static const double reduced_opacity = 0.4;

    double opacity = title.empty() ? reduced_opacity : default_opacity;
    const Glib::ustring& profile_title = title.empty() ? profile_title_placeholder_ : title;

    current_profile_label_->set_opacity(opacity);
    current_profile_label_->set_text(profile_title);
    current_profile_label_->show();
    Gtk::Window::set_title(Glib::get_application_name() + " - " + profile_title);
  }
  else
  {
    current_profile_label_->hide();
    current_profile_label_->set_text("");
    Gtk::Window::set_title(Glib::get_application_name());
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
  std::size_t next_accent = stats.next_accent;
  if ( next_accent < accent_button_grid_.size() )
  {
    std::chrono::microseconds time = stats.timestamp
      + stats.backend_latency
      + stats.next_accent_delay;

    time += animation_sync_;

    accent_button_grid_[next_accent].scheduleAnimation(time.count());
  }
}

void MainWindow::updateCurrentTempo(const audio::Ticker::Statistics& stats)
{
  static const Glib::ustring kAccelUpSymbol     = "\xe2\x96\xb4"; // "▴"
  static const Glib::ustring kAccelDownSymbol   = "\xe2\x96\xbe"; // "▾"
  static const Glib::ustring kAccelStableSymbol = "\xe2\x80\xa2"; // "•"

  static const int precision = 2;

  double tempo_integral;
  double tempo_fraction;

  tempo_fraction = std::modf(stats.current_tempo, &tempo_integral);

  int tempo_integral_int = tempo_integral;
  int tempo_fraction_int = std::round(tempo_fraction * std::pow(10, precision));

  if (tempo_fraction_int == 100)
  {
    tempo_fraction_int = 0;
    tempo_integral_int += 1;
  }
  auto text = Glib::ustring::format(tempo_integral_int);

  if (text != tempo_integral_label_->get_text())
    tempo_integral_label_->set_text(text);

  text = Glib::ustring::format(std::setfill(L'0'), std::setw(precision), tempo_fraction_int);

  if (text != tempo_fraction_label_->get_text())
    tempo_fraction_label_->set_text(text);

  if (stats.current_accel == 0)
    text = kAccelStableSymbol;
  else if (stats.current_accel > 0)
    text = kAccelUpSymbol;
  else
    text = kAccelDownSymbol;

  if (text != tempo_divider_label_->get_text())
    tempo_divider_label_->set_text(text);
}

void MainWindow::updatePendulum(const audio::Ticker::Statistics& stats)
{
  pendulum_.synchronize(stats, animation_sync_);
}

void MainWindow::onTickerStatistics(const audio::Ticker::Statistics& stats)
{
  updateCurrentTempo(stats);

  if (meter_animation_)
    updateAccentAnimation(stats);

  updatePendulum(stats);
}

void MainWindow::onTap(double confidence)
{
  tap_level_bar_->set_value(confidence);

  if (!isTapAnimationTimerRunning())
    startTapAnimationTimer();
}

namespace {
  constexpr unsigned int kTapAnimationTimerInterval = 100; // ms
  constexpr double kTapAnimationFallOffVelocity = 0.8; // units per second
}//unnamed namespace

void MainWindow::startTapAnimationTimer()
{
  if (!isTapAnimationTimerRunning())
  {
    tap_animation_timer_connection_ = Glib::signal_timeout()
      .connect(sigc::mem_fun(*this, &MainWindow::onTapAnimationTimer),
               kTapAnimationTimerInterval);
  }
}

void MainWindow::stopTapAnimationTimer()
{
  if (isTapAnimationTimerRunning())
  {
    tap_animation_timer_connection_.disconnect();
    tap_level_bar_->set_value(0.0);
  }
}

bool MainWindow::isTapAnimationTimerRunning()
{
  return tap_animation_timer_connection_.connected();
}

bool MainWindow::onTapAnimationTimer()
{
  double value = tap_level_bar_->get_value();

  value -= kTapAnimationFallOffVelocity * kTapAnimationTimerInterval / 1000.0;
  value = std::clamp(value, 0.0, 1.0);

  tap_level_bar_->set_value(value);

  return value > 0.0;
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
  if (key == settings::kKeyPrefsPendulumAction)
    updatePrefPendulumAction();
  else if (key == settings::kKeyPrefsPendulumPhaseMode)
    updatePrefPendulumPhaseMode();
  else if (key == settings::kKeyPrefsAnimationSync)
    updatePrefAnimationSync();
  else if (key == settings::kKeyPrefsMeterAnimation)
    updatePrefMeterAnimation();
}

void MainWindow::updatePrefPendulumAction()
{
  int action = settings::preferences()->get_enum(settings::kKeyPrefsPendulumAction);
  switch (action)
  {
  case settings::kPendulumActionCenter:
    pendulum_.setAction( Pendulum::ActionAngle::kCenter );
    break;
  case settings::kPendulumActionReal:
    pendulum_.setAction( Pendulum::ActionAngle::kReal );
    break;
  case settings::kPendulumActionEdge:
    pendulum_.setAction( Pendulum::ActionAngle::kEdge );
    break;
  default:
    pendulum_.setAction( Pendulum::ActionAngle::kReal );
    break;
  };
}

void MainWindow::updatePrefPendulumPhaseMode()
{
  int mode = settings::preferences()->get_enum(settings::kKeyPrefsPendulumPhaseMode);
  switch (mode)
  {
  case settings::kPendulumPhaseModeLeft:
    pendulum_.setPhaseMode( Pendulum::PhaseMode::kLeft );
    break;
  case settings::kPendulumPhaseModeRight:
    pendulum_.setPhaseMode( Pendulum::PhaseMode::kRight );
    break;
  default:
    pendulum_.setPhaseMode( Pendulum::PhaseMode::kLeft );
    break;
  };
}

void MainWindow::updatePrefMeterAnimation()
{
  meter_animation_ = settings::preferences()->get_boolean(settings::kKeyPrefsMeterAnimation);
}

void MainWindow::updatePrefAnimationSync()
{
  animation_sync_ = std::chrono::microseconds(
    std::lround(settings::preferences()->get_double(settings::kKeyPrefsAnimationSync) * 1000.));
}
