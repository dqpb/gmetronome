/*
 * Copyright (C) 2020-2024 The GMetronome Team
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

//static
Glib::ustring MainWindow::duplicateDocumentTitle(const Glib::ustring& title_old,
                                                 const Glib::ustring& title_duplicate_fmt,
                                                 const Glib::ustring& title_placeholder)
{
  Glib::ustring title_new;

  if (!title_old.empty())
  {
    // compose a new title from the old title;
    // if the current title is itself a composition we don't change it
    const auto regex = Glib::Regex::create("%1");
    const auto pattern
      = Glib::ustring("\\A")
      + regex->replace(Glib::Regex::escape_string(title_duplicate_fmt), 0,
                       ".*", static_cast<Glib::RegexMatchFlags>(0))
      + Glib::ustring("\\Z");

    if (!Glib::Regex::match_simple(pattern, title_old))
      title_new = Glib::ustring::compose(title_duplicate_fmt, title_old);
    else
      title_new = title_old;
  }
  else // old title is empty
  {
    // compose the duplicate from the placeholder title
    title_new = Glib::ustring::compose(title_duplicate_fmt,
                                       title_placeholder);
  }
  return title_new;
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

  app_ = Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default());

  builder_->get_widget("headerBar", header_bar_);
  builder_->get_widget("headerBarTitleBox", header_bar_title_box_);
  builder_->get_widget("currentProfileLabel", current_profile_label_);
  builder_->get_widget("fullScreenButton", full_screen_button_);
  builder_->get_widget("fullScreenImage", full_screen_image_);
  builder_->get_widget("mainMenuButton", main_menu_button_);
  builder_->get_widget("popoverMenu", popover_menu_);
  builder_->get_widget("profileMenuButton", profile_menu_button_);
  builder_->get_widget("profilePopover", profile_popover_);
  builder_->get_widget("profileMainBox", profile_main_box_);
  builder_->get_widget("profileHeaderBox", profile_header_box_);
  builder_->get_widget("profileScrolledWindow", profile_scrolled_window_);
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
  builder_->get_widget("mainContentBox", main_content_box_);
  builder_->get_widget("volumeButton", volume_button_);
  builder_->get_widget("startButton", start_button_);
  builder_->get_widget("trainerToggleButton", trainer_toggle_button_);
  builder_->get_widget("accentToggleButton", accent_toggle_button_);
  builder_->get_widget("trainerRevealer", trainer_revealer_);
  builder_->get_widget("accentRevealer", accent_revealer_);
  builder_->get_widget("pendulumRevealer", pendulum_revealer_);
  builder_->get_widget("pendulumContentBox", pendulum_content_box_);
  builder_->get_widget("trainerFrame", trainer_frame_);
  builder_->get_widget("accentFrame", accent_frame_);
  builder_->get_widget("accentContentBox", accent_content_box_);
  builder_->get_widget("tempoScale", tempo_scale_);
  builder_->get_widget("tempoSpinButton", tempo_spin_button_);
  builder_->get_widget("tapEventBox", tap_event_box_);
  builder_->get_widget("tapBox", tap_box_);
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
  builder_->get_widget("trainerStack", trainer_stack_);
  builder_->get_widget("trainerModeButtonBox", trainer_mode_button_box_);
  builder_->get_widget("trainerMode1RadioButton", trainer_mode_1_radio_button_);
  builder_->get_widget("trainerMode2RadioButton", trainer_mode_2_radio_button_);

  tempo_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("tempoAdjustment"));

  trainer_target_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerTargetAdjustment"));

  trainer_accel_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerAccelAdjustment"));

  trainer_step_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerStepAdjustment"));

  trainer_hold_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("trainerHoldAdjustment"));

  beats_adjustment_ = Glib::RefPtr<Gtk::Adjustment>
    ::cast_dynamic(builder_->get_object("beatsAdjustment"));

  accent_button_grid_.set_name("accentButtonGrid");
  accent_button_grid_.set_margin_start(20);
  accent_button_grid_.set_margin_end(20);
  accent_content_box_->pack_start(accent_button_grid_);
  accent_button_grid_.show();

  profile_title_default_ =
    g_dpgettext2(NULL, "Profile", Profile::kDefaultTitle.c_str());
  profile_title_duplicate_ =
    g_dpgettext2(NULL, "Profile", Profile::kDefaultTitleDuplicate.c_str());
  profile_title_placeholder_ =
    g_dpgettext2(NULL, "Profile", Profile::kDefaultTitlePlaceholder.c_str());

  profile_list_store_ = ProfileListStore::create();
  profile_tree_view_->set_model(profile_list_store_);
  profile_tree_view_->append_column_editable("Title", profile_list_store_->columns_.title_);
  profile_tree_view_->get_column(0)->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
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
  const ActionHandlerList kWinActionHandler =
    {
      {kActionShowPrimaryMenu,         sigc::mem_fun(*this, &MainWindow::onShowPrimaryMenu)},
      {kActionShowProfiles,            sigc::mem_fun(*this, &MainWindow::onShowProfiles)},
      {kActionShowPreferences,         sigc::mem_fun(*this, &MainWindow::onShowPreferences)},
      {kActionShowShortcuts,           sigc::mem_fun(*this, &MainWindow::onShowShortcuts)},
      {kActionShowHelp,                sigc::mem_fun(*this, &MainWindow::onShowHelp)},
      {kActionShowAbout,               sigc::mem_fun(*this, &MainWindow::onShowAbout)},
      {
        kActionShowPendulum,
        sigc::mem_fun(*this, &MainWindow::onShowPendulum),
        settings::state()
      },
      {kActionFullScreen,              sigc::mem_fun(*this, &MainWindow::onToggleFullScreen)},
      {kActionPendulumTogglePhase,     sigc::mem_fun(*this, &MainWindow::onPendulumTogglePhase)},
      {kActionTempoQuickSet,           sigc::mem_fun(*this, &MainWindow::onTempoQuickSet)}
    };

  install_actions(*this, kWinActionHandler);
}

void MainWindow::initUI()
{
  // initialize title bar
  titlebar_bin_.add(*header_bar_);
  set_titlebar(titlebar_bin_);
  titlebar_bin_.show();

  // initialize header bar
  header_bar_title_box_->pack_start(tempo_display_, Gtk::PACK_EXPAND_WIDGET);
  tempo_display_.set_name("tempoDisplay");
  tempo_display_.show();
  updateCurrentTempo(audio::Ticker::Statistics{});

  // initialize info bar
  info_overlay_->add_overlay(*info_revealer_);
  info_revealer_->set_reveal_child(false);

  // initialize about dialog
  about_dialog_.set_transient_for(*this);

  // initialize pendulum
  pendulum_content_box_->pack_start(pendulum_, Gtk::PACK_EXPAND_WIDGET);
  pendulum_.set_halign(Gtk::ALIGN_CENTER);
  pendulum_.show();

  // initialize tempo interface
  if (tempo_scale_->get_direction() != Gtk::TEXT_DIR_RTL) // Gtk::Scale marks seem not to work
  {                                                       // properly for RTL languages
    Glib::ustring mark_30  = Glib::ustring::format(30);
    Glib::ustring mark_120 = Glib::ustring::format(120);
    Glib::ustring mark_250 = Glib::ustring::format(250);

    tempo_scale_->add_mark(30.0, Gtk::POS_BOTTOM, mark_30);
    tempo_scale_->add_mark(120.0, Gtk::POS_BOTTOM, mark_120);
    tempo_scale_->add_mark(250.0, Gtk::POS_BOTTOM, mark_250);
  }
  tempo_scale_->set_round_digits(0);

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

  Glib::ustring meter_slot = app_->queryMeterSelect();
  updateMeter(meter_slot, app_->queryMeter(meter_slot));

  // initialize transport interface
  updateStartButtonLabel(false);
  updateVolumeMute(false);

  // initialize profile list
  updateProfileList(app_->queryProfileList());

  // initalize profile selection
  Glib::ustring id = app_->queryProfileSelect();
  updateProfileSelect(id);

  // initialize profile title
  Glib::ustring title = app_->queryProfileTitle();
  updateProfileTitle(title, !id.empty());
}

void MainWindow::initBindings()
{
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

  tap_event_box_->add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);

  tap_event_box_->signal_button_press_event().connect(
    [&] (GdkEventButton* button_event) {
      if (button_event->type != GDK_2BUTTON_PRESS && button_event->type != GDK_3BUTTON_PRESS)
      {
        Gtk::Application::get_default()->activate_action(kActionTempoTap);
        tap_box_->set_state_flags(Gtk::STATE_FLAG_ACTIVE);
      }
      return true;
    });

  tap_event_box_->signal_button_release_event().connect(
    [&] (GdkEventButton* button_event) {
      tap_box_->set_state_flags(Gtk::STATE_FLAG_NORMAL);
      return true;
    });

  action_bindings_
    .push_back( bind_action(app_,
                            kActionTempo,
                            tempo_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app_,
                            kActionTrainerTarget,
                            trainer_target_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app_,
                            kActionTrainerAccel,
                            trainer_accel_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app_,
                            kActionTrainerStep,
                            trainer_step_adjustment_->property_value()) );
  action_bindings_
    .push_back( bind_action(app_,
                            kActionTrainerHold,
                            trainer_hold_adjustment_->property_value()) );

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

  trainer_mode_1_radio_button_->signal_clicked()
    .connect([&]{onTrainerModeChanged(trainer_mode_1_radio_button_);});

  trainer_mode_2_radio_button_->signal_clicked()
    .connect([&]{onTrainerModeChanged(trainer_mode_2_radio_button_);});

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

  app_->signal_action_state_changed()
    .connect(sigc::mem_fun(*this, &MainWindow::onActionStateChanged));

  profile_popover_->signal_show()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileShow));

  profile_popover_->signal_hide()
    .connect(sigc::mem_fun(*this, &MainWindow::onProfileHide));

  app_->signalMessage()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessage));

  info_bar_->signal_response()
    .connect(sigc::mem_fun(*this, &MainWindow::onMessageResponse));

  app_->signalTickerStatistics()
    .connect(sigc::mem_fun(*this, &MainWindow::onTickerStatistics));

  app_->signalTap()
    .connect(sigc::mem_fun(*this, &MainWindow::onTap));
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

bool MainWindow::on_configure_event(GdkEventConfigure* configure_event)
{
  Gtk::Window::on_configure_event(configure_event);

  if (profile_popover_->is_visible())
    resizeProfilePopover();

  return false;
}

namespace {
  constexpr unsigned int kTempoQuickSetTimerTimeout  = 1600; // ms
  constexpr unsigned int kTempoQuickSetTimerInterval = 70;
}//unnamed namespace

void MainWindow::startTempoQuickSetTimer()
{
  if (!isTempoQuickSetTimerRunning())
  {
    resetTempoQuickSetTimerTimeout();

    tempo_quick_set_timer_connection_ = Glib::signal_timeout()
      .connect(sigc::mem_fun(*this, &MainWindow::onTempoQuickSetTimer),
               kTempoQuickSetTimerInterval);
  }
}

void MainWindow::stopTempoQuickSetTimer(bool accept)
{
  if (isTempoQuickSetTimerRunning())
  {
    tempo_spin_button_->set_progress_fraction(0.0);
    tempo_quick_set_timer_connection_.disconnect();
  }
}

void MainWindow::resetTempoQuickSetTimerTimeout()
{
  tempo_quick_set_timer_timeout_ = kTempoQuickSetTimerTimeout;
}

bool MainWindow::isTempoQuickSetTimerRunning()
{
  return tempo_quick_set_timer_connection_.connected();
}

bool MainWindow::onTempoQuickSetTimer()
{
  tempo_quick_set_timer_timeout_ -= kTempoQuickSetTimerInterval;

  if (tempo_quick_set_timer_timeout_ > 0)
  {
    tempo_spin_button_->set_progress_fraction(
      (double)tempo_quick_set_timer_timeout_ / kTempoQuickSetTimerTimeout );

    return true;
  }
  else
  {
    acceptTempoQuickSetEditing();
    return false;
  }
}

bool MainWindow::handleTempoQuickSetKeyEvent(GdkEventKey* key_event)
{
  bool event_handled = false;

  if (isTempoQuickSetEditing())
  {
    if (key_event->keyval == GDK_KEY_Escape)
    {
      abortTempoQuickSetEditing();
      event_handled = true;
    }
    else if (key_event->keyval == GDK_KEY_BackSpace
             || key_event->keyval == GDK_KEY_Delete)
    {
      if (int start_pos = tempo_spin_button_->get_position(); start_pos > 0)
      {
        tempo_spin_button_->set_editable(true);
        tempo_spin_button_->delete_text(start_pos - 1, -1);
        tempo_spin_button_->set_editable(false);
      }
      event_handled = true;
    }
    else if (key_event->keyval == GDK_KEY_Return
             || key_event->keyval == GDK_KEY_ISO_Enter
             || key_event->keyval == GDK_KEY_KP_Enter)
    {
      acceptTempoQuickSetEditing();
      event_handled = true;
    }
    else
    {
      tempo_spin_button_->set_editable(true);
      if (tempo_spin_button_->im_context_filter_keypress(key_event))
      {
        tempo_spin_button_->set_position(-1);
        event_handled = true;
      }
      else
      {
        gchar* accel_name =
          gtk_accelerator_name(key_event->keyval, (GdkModifierType)key_event->state);

        if (auto actions = Gtk::Window::get_application()->get_actions_for_accel(accel_name);
            actions.empty())
          event_handled = true;

        delete [] accel_name;
      }
      tempo_spin_button_->set_editable(false);
    }
  }

  if (event_handled)
    resetTempoQuickSetTimerTimeout();

  return event_handled;
}

bool MainWindow::startTempoQuickSetEditing()
{
  if (isTempoQuickSetEditing())
    return false;

  tempo_spin_button_->set_placeholder_text(tempo_spin_button_->get_text());
  tempo_spin_button_->delete_text(0,-1);
  tempo_spin_button_->reset_im_context();

  // During a tempo 'quick set' session, the spin button is not 'editable'.
  // This prevents some unwanted side effects, like changing the value, if
  // the spinbutton loses the focus etc.
  tempo_spin_button_->set_editable(false);

  // After changing the value of 'editable', GTK does not redraw the buttons
  // of the spinbutton immediately to indicate the changed sensitivity. Even
  // calling queue_redraw() does not have the desired effect. To force an
  // update nevertheless, we reset the range of the spinbutton.
  tempo_spin_button_->set_range(tempo_adjustment_->get_lower(),
                                tempo_adjustment_->get_upper());

  tempo_quick_set_editing_ = true;

  startTempoQuickSetTimer();

  return true;
}

void MainWindow::acceptTempoQuickSetEditing()
{
  if (!isTempoQuickSetEditing())
    return;

  if (tempo_spin_button_->get_position() > 0)
  {
    tempo_quick_set_editing_ = false;

    tempo_spin_button_->set_editable(true);
    tempo_spin_button_->set_range(tempo_adjustment_->get_lower(),
                                  tempo_adjustment_->get_upper());

    tempo_spin_button_->activate();
    tempo_spin_button_->set_placeholder_text("");

    stopTempoQuickSetTimer(true);
  }
  else
    abortTempoQuickSetEditing();
}

void MainWindow::abortTempoQuickSetEditing()
{
  if (!isTempoQuickSetEditing())
    return;

  tempo_spin_button_->set_editable(true);
  tempo_spin_button_->set_range(tempo_adjustment_->get_lower(),
                                tempo_adjustment_->get_upper());

  // force reload number from adjustment
  tempo_spin_button_->set_adjustment(tempo_adjustment_);
  tempo_spin_button_->set_placeholder_text("");

  tempo_quick_set_editing_ = false;

  stopTempoQuickSetTimer(false);
}

bool MainWindow::isTempoQuickSetEditing() const
{
  return (tempo_quick_set_editing_ == true);
}

bool MainWindow::on_key_press_event(GdkEventKey* key_event)
{
  if (handleTempoQuickSetKeyEvent(key_event))
    return true;
  else
    return Gtk::Widget::on_key_press_event(key_event);
}

int MainWindow::estimateProfileTreeViewRowHeight() const
{
  Gtk::TreeViewColumn* col = profile_tree_view_->get_column(0);

  Gdk::Rectangle rect(0,0,0,0);
  int cell_x_offset, cell_y_offset, cell_width, cell_height;

  col->cell_get_size(rect, cell_x_offset, cell_y_offset, cell_width, cell_height);

  int xpad, ypad;
  Gtk::CellRenderer* renderer = col->get_first_cell();
  renderer->get_padding(xpad, ypad);

  int row_height = cell_height + ypad;

  return row_height;
}

void MainWindow::resizeProfilePopover(bool process_pending)
{
  static const Gtk::Requisition kPopoverPreferredMinSize {220, 260};

  if (process_pending)
  {
    // under certain circumstances this may be necessary to update
    // widget sizes, especially the natural size of the tree view
    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  Gtk::Requisition win_size;
  Gtk::Window::get_size(win_size.width, win_size.height);

  Gtk::Requisition header_min_size, header_nat_size;
  profile_header_box_->get_preferred_size(header_min_size, header_nat_size);

  Gtk::Requisition tv_min_size, tv_nat_size;
  profile_tree_view_->get_preferred_size(tv_min_size, tv_nat_size);

  int tv_row_height = estimateProfileTreeViewRowHeight();

  int po_height =
    std::min(win_size.height, std::max(kPopoverPreferredMinSize.height,
                                       header_nat_size.height
                                       + tv_nat_size.height
                                       + tv_row_height
                                       + 50));
  int po_width =
    std::min(win_size.width, std::max(kPopoverPreferredMinSize.width,
                                      tv_nat_size.width + 50));

  profile_popover_->set_size_request(po_width, po_height);
}

void MainWindow::onProfileShow()
{
  profile_tree_view_->set_can_focus(true);

  if (profile_tree_view_->get_selection()->count_selected_rows() != 0)
    profile_tree_view_->property_has_focus() = true;
  else
    profile_new_button_->property_has_focus() = true;

  resizeProfilePopover();
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
    "        <property name=\"max-height\">11</property>\n";

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

  static const Glib::ustring ui_shortcut =
    "            <child>\n"
    "              <object class=\"GtkShortcutsShortcut\">\n"
    "                <property name=\"visible\">1</property>\n"
    "                <property name=\"accelerator\">%1</property>\n"
    "                <property name=\"title\">%2</property>\n"
    "              </object>\n"
    "            </child>\n";

  Glib::ustring ui = ui_header + ui_section_header;

  for (const auto& [group_id, group_title, group_shortcuts] : ShortcutList())
  {
    bool group_open = false;

    for (const auto& [shortcut_key, shortcut_title] : group_shortcuts)
    {
      auto accel = settings::shortcuts()->get_string(shortcut_key);

      // validate accelerator
      guint accel_key;
      GdkModifierType accel_mods;
      gtk_accelerator_parse(accel.c_str(), &accel_key, &accel_mods);

      if (accel_key != 0 || accel_mods != 0)
      {
        if (!group_open)
        {
          ui += Glib::ustring::compose(ui_group_header, Glib::Markup::escape_text(group_title));
          group_open = true;
        }
        ui += Glib::ustring::compose( ui_shortcut,
                                      Glib::Markup::escape_text(accel),
                                      Glib::Markup::escape_text(shortcut_title));
      }
    }
    if (group_open)
      ui += ui_group_footer;
  }

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
  about_dialog_.show();
  about_dialog_.present();
}

void MainWindow::onShowPendulum(const Glib::VariantBase& parameter)
{}

void MainWindow::onToggleFullScreen(const Glib::VariantBase& parameter)
{
  auto new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(parameter);

  if (new_state.get())
    fullscreen();
  else
    unfullscreen();
}

void MainWindow::onPendulumTogglePhase(const Glib::VariantBase& value)
{
  pendulum_.togglePhase();
}

void MainWindow::onTempoQuickSet(const Glib::VariantBase& value)
{
  startTempoQuickSetEditing();
}

void MainWindow::activateMeterAction(const Glib::ustring& action,
                                     const Glib::VariantBase& param)
{
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

  app_->activate_action(action, param);

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
  Glib::ustring meter_slot = meter_combo_box_->get_active_id();
  Meter meter = app_->queryMeter(meter_slot);
  int beats = std::lround(beats_adjustment_->get_value());

  meter.setBeats(beats);

  auto new_state = Glib::Variant<Meter>::create(meter);
  activateMeterAction(meter_slot, new_state);
}

void MainWindow::onSubdivChanged(Gtk::RadioButton* button, int division)
{
  if (!button->get_active())
    return;

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();
  Meter meter = app_->queryMeter(meter_slot);

  meter.setDivision(division);

  auto new_state = Glib::Variant<Meter>::create(meter);
  activateMeterAction(meter_slot, new_state);
}

void MainWindow::onAccentChanged(std::size_t button_index)
{
  const Meter& meter = accent_button_grid_.meter();

  auto state = Glib::Variant<Meter>::create(meter);

  Glib::ustring meter_slot = meter_combo_box_->get_active_id();
  app_->activate_action(meter_slot, state);
}

void MainWindow::onTrainerModeChanged(Gtk::RadioButton* button)
{
  if (button == nullptr || !button->get_active())
    return;

  Profile::TrainerMode mode;

  if (button == trainer_mode_1_radio_button_)
    mode = Profile::TrainerMode::kContinuous;
  else if (button == trainer_mode_2_radio_button_)
    mode = Profile::TrainerMode::kStepwise;
  else
    return;

  auto mode_variant = Glib::Variant<Profile::TrainerMode>::create(mode);
  app_->activate_action(kActionTrainerMode, mode_variant);
}

void MainWindow::onProfileSelectionChanged()
{
  Glib::ustring id;

  auto row_it = profile_tree_view_->get_selection()->get_selected();

  if (row_it)
    id = row_it->get_value(profile_list_store_->columns_.id_);

  auto state = Glib::Variant<Glib::ustring>::create(id);

  app_->activate_action(kActionProfileSelect, state);
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
  Gtk::TreePath path(path_string);

  ProfileListStore::iterator row_it = profile_list_store_->get_iter(path);
  if(row_it)
  {
    if (Glib::ustring selected_id = app_->queryProfileSelect();
        selected_id == (*row_it)[profile_list_store_->columns_.id_])
    {
      auto state = Glib::Variant<Glib::ustring>::create(text);
      app_->activate_action(kActionProfileTitle, state);
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

  app_->activate_action(kActionProfileReorder,
                        Glib::Variant<ProfileIdentifierList>::create(id_list));

  updateProfileSelect(app_->queryProfileSelect());
}

void MainWindow::onProfileNew()
{
  Glib::ustring new_title;
  if (auto id = app_->queryProfileSelect(); !id.empty())
  {
    new_title = duplicateDocumentTitle(app_->queryProfileTitle(),
                                       profile_title_duplicate_,
                                       profile_title_placeholder_);
  }
  else // no selected profile
    new_title = profile_title_default_;

  const auto new_title_variant = Glib::Variant<Glib::ustring>::create(new_title);
  app_->activate_action(kActionProfileNew, new_title_variant);
}

void MainWindow::onActionStateChanged(const Glib::ustring& action_name,
                                      const Glib::VariantBase& variant)
{
  if (isTempoQuickSetEditing())
    abortTempoQuickSetEditing();

  if (action_name.compare(0,6,"meter-") == 0)
  {
    Glib::ustring meter_slot = app_->queryMeterSelect();
    if (action_name == kActionMeterSelect || action_name == meter_slot)
    {
      updateMeter(meter_slot, app_->queryMeter(meter_slot));
    }
  }
  else if (action_name.compare(kActionTempo) == 0)
  {
    updateTempo(app_->queryTempo());
  }
  else if (action_name.compare(kActionStart) == 0)
  {
    updateStart(app_->queryStart());
  }
  else if (action_name.compare(kActionTrainerMode) == 0)
  {
    updateTrainerMode(app_->queryTrainerMode());
  }
  else if (action_name.compare(kActionProfileList) == 0)
  {
    updateProfileList(app_->queryProfileList());
    updateProfileSelect(app_->queryProfileSelect());

    if (profile_popover_->is_visible())
      resizeProfilePopover(true);
  }
  else if (action_name.compare(kActionProfileSelect) == 0)
  {
    Glib::ustring id = app_->queryProfileSelect();
    updateProfileSelect(id);

    // switching from a profile-less state to an untitled profile does not
    // change the state of kActionProfileTitle, but requires to update
    // the title nevertheless
    if (auto title = app_->queryProfileTitle(); title.empty())
      updateProfileTitle(title, !id.empty());
  }
  else if (action_name.compare(kActionProfileTitle) == 0)
  {
    Glib::ustring id = app_->queryProfileSelect();
    Glib::ustring title = app_->queryProfileTitle();

    updateProfileTitle(title, !id.empty());
  }
  else if (action_name.compare(kActionVolumeMute) == 0)
  {
    updateVolumeMute(app_->queryVolumeMute());
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

  accent_button_grid_.setMeter(meter);

  std::for_each(meter_connections_.begin(), meter_connections_.end(),
                std::mem_fn(&sigc::connection::unblock));
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
    {
      Gtk::TreePath path(it);
      profile_tree_view_->set_cursor(path);
      profile_tree_view_->scroll_to_row(path);
    }
    else
      profile_tree_view_->get_selection()->unselect_all();
  }
  profile_selection_changed_connection_.unblock();
}

void MainWindow::updateProfileTitle(const Glib::ustring& title, bool has_profile)
{
  if (has_profile)
  {
    bool is_placeholder = title.empty() ? true : false;
    const Glib::ustring& profile_title = is_placeholder ? profile_title_placeholder_ : title;

    auto style_context = current_profile_label_->get_style_context();
    if (is_placeholder)
    {
      if (!style_context->has_class("placeholder"))
        style_context->add_class("placeholder");
    }
    else if (style_context->has_class("placeholder"))
      style_context->remove_class("placeholder");

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
  // nothing
}

void MainWindow::updateTrainerMode(Profile::TrainerMode mode)
{
  if (mode == Profile::TrainerMode::kContinuous)
  {
    if (!trainer_mode_1_radio_button_->get_active())
      trainer_mode_1_radio_button_->set_active(true);

    trainer_stack_->set_visible_child("trainerContinuousPage");
  }
  else if (mode == Profile::TrainerMode::kStepwise)
  {
    if (!trainer_mode_2_radio_button_->get_active())
      trainer_mode_2_radio_button_->set_active(true);

    trainer_stack_->set_visible_child("trainerStepwisePage");
  }
}

void MainWindow::updateStart(bool running)
{
  if (running)
  {
    pendulum_.start();
    accent_button_grid_.start();
  }
  else
  {
    pendulum_.stop();
    accent_button_grid_.stop();
  }
  updateStartButtonLabel(running);
}

void MainWindow::updateStartButtonLabel(bool running)
{
  if (running)
    start_button_->set_label(C_("Main window", "Stop"));
  else
    start_button_->set_label(C_("Main window", "Start"));
}

void MainWindow::updateVolumeMute(bool mute)
{
  static const std::vector<Glib::ustring> icons_muted = {
    "gm-snd-volume-muted-symbolic"
  };
  static const std::vector<Glib::ustring> icons_unmuted = {
    "gm-snd-volume-zero-symbolic",
    "gm-snd-volume-full-symbolic",
    "gm-snd-volume-low-symbolic",
    "gm-snd-volume-medium-symbolic",
    "gm-snd-volume-high-symbolic"
  };

  if (mute)
  {
    volume_button_->set_icons(icons_muted);
  }
  else
  {
    volume_button_->set_icons(icons_unmuted);
  }
}

void MainWindow::updateCurrentTempo(const audio::Ticker::Statistics& stats)
{
  tempo_display_.display(stats.tempo, stats.acceleration);
}

void MainWindow::updateAccentAnimation(const audio::Ticker::Statistics& stats)
{
  accent_button_grid_.synchronize(stats, animation_sync_);
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
