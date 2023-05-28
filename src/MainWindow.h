/*
 * Copyright (C) 2020-2023 The GMetronome Team
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

#ifndef GMetronome_MainWindow_h
#define GMetronome_MainWindow_h

#include "Action.h"
#include "Ticker.h"
#include "Message.h"
#include "Pendulum.h"
#include "AccentButtonGrid.h"
#include "About.h"

#include <gtkmm.h>
#include <list>
#include <vector>
#include <chrono>

class ActionBinding;
class ProfileListStore;
class SettingsDialog;
class AccentButtonGrid;

class MainWindow : public Gtk::ApplicationWindow
{
public:
  MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainWindow();

  static MainWindow* create();

private:
  Glib::RefPtr<Gtk::Builder> builder_;

  //Bindings
  std::list<Glib::RefPtr<Glib::Binding>> bindings_;
  std::list<Glib::RefPtr<ActionBinding>> action_bindings_;

  // Connections
  std::vector<sigc::connection> meter_connections_;
  sigc::connection profile_selection_changed_connection_;
  sigc::connection profile_popover_show_connection_;
  sigc::connection pendulum_restore_connection_;
  sigc::connection tap_animation_timer_connection_;

  // Dialogs
  SettingsDialog* preferences_dialog_;
  GMetronomeAboutDialog about_dialog_;
  Gtk::ShortcutsWindow* shortcuts_window_;

  // UI elements
  class HeaderBarBin : public Gtk::Bin {} titlebar_bin_;
  Gtk::HeaderBar* header_bar_;
  Gtk::Label* tempo_integral_label_;
  Gtk::Label* tempo_fraction_label_;
  Gtk::Label* tempo_divider_label_;
  Gtk::Label* current_profile_label_;
  Gtk::Button* full_screen_button_;
  Gtk::Image* full_screen_image_;
  Gtk::MenuButton* main_menu_button_;
  Gtk::PopoverMenu* popover_menu_;
  Gtk::MenuButton* profile_menu_button_;
  Gtk::Popover* profile_popover_;
  Gtk::TreeView* profile_tree_view_;
  Gtk::Button* profile_new_button_;
  Gtk::Button* profile_delete_button_;
  Gtk::Box* main_box_;
  Gtk::Overlay* info_overlay_;
  Gtk::Revealer* info_revealer_;
  Gtk::InfoBar* info_bar_;
  Gtk::Box* info_content_box_;
  Gtk::ButtonBox* info_button_box_;
  Gtk::Image* info_image_;
  Gtk::Box* info_label_box_;
  Gtk::Label* info_topic_label_;
  Gtk::Label* info_text_label_;
  Gtk::Label* info_details_label_;
  Gtk::Expander* info_details_expander_;
  Gtk::Box* content_box_;
  Gtk::ScaleButton* volume_button_;
  Gtk::ToggleButton* start_button_;
  Gtk::ToggleButton* trainer_toggle_button_;
  Gtk::ToggleButton* accent_toggle_button_;
  Gtk::Revealer* trainer_revealer_;
  Gtk::Revealer* accent_revealer_;
  Gtk::Revealer* pendulum_revealer_;
  Gtk::Box* pendulum_box_;
  Gtk::Frame* trainer_frame_;
  Gtk::Frame* accent_frame_;
  Gtk::Box* accent_box_;
  Gtk::Scale* tempo_scale_;
  Gtk::EventBox* tap_event_box_;
  Gtk::LevelBar* tap_level_bar_;
  Gtk::ComboBoxText* meter_combo_box_;
  Gtk::SpinButton* beats_spin_button_;
  Gtk::Label* beats_label_;
  Gtk::ButtonBox* subdiv_button_box_;
  Gtk::RadioButton* subdiv_1_radio_button_;
  Gtk::RadioButton* subdiv_2_radio_button_;
  Gtk::RadioButton* subdiv_3_radio_button_;
  Gtk::RadioButton* subdiv_4_radio_button_;
  Gtk::Label* subdiv_label_;
  AccentButtonGrid accent_button_grid_;
  Pendulum pendulum_;

  Glib::RefPtr<Gtk::Adjustment> tempo_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_start_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_target_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_accel_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> beats_adjustment_;

  Glib::RefPtr<ProfileListStore> profile_list_store_;

  Glib::ustring profile_title_new_;
  Glib::ustring profile_title_placeholder_;

  // cached preferences
  bool meter_animation_;
  std::chrono::microseconds animation_sync_;

  bool bottom_resizable_;
  gint64 last_meter_action_;

private:
  // Initialization
  void initActions();
  void initUI();
  void initBindings();

  // Override window signal handler
  bool on_window_state_event(GdkEventWindowState* window_state_event) override;

  // Window actions
  void onShowPrimaryMenu(const Glib::VariantBase& value);
  void onShowProfiles(const Glib::VariantBase& value);
  void onShowPreferences(const Glib::VariantBase& value);
  void onShowShortcuts(const Glib::VariantBase& value);
  void onShowHelp(const Glib::VariantBase& value);
  void onShowAbout(const Glib::VariantBase& value);
  void onToggleFullScreen(const Glib::VariantBase& value);

  // UI handler
  bool onTempoTap(GdkEventButton* button_event);
  void onTempoLabelAllocate(Gtk::Allocation& alloc);
  void activateMeterAction(const Glib::ustring& action, const Glib::VariantBase& param);
  void onMeterChanged();
  void onBeatsChanged();
  void onSubdivChanged(Gtk::RadioButton* button, int division);
  void onAccentChanged(std::size_t button_index);
  void onProfileSelectionChanged();
  void onProfileTitleStartEditing(Gtk::CellEditable* editable, const Glib::ustring& path);
  void onProfileTitleChanged(const Glib::ustring& path, const Glib::ustring& new_text);
  void onProfileDragBegin(const Glib::RefPtr<Gdk::DragContext>& context);
  void onProfileDragEnd(const Glib::RefPtr<Gdk::DragContext>& context);
  void onProfileNew();
  void onProfileShow();
  void onProfileHide();

  // Action handler
  void onActionStateChanged(const Glib::ustring& action_name,
                            const Glib::VariantBase& variant);

  void updateMeter(const Glib::ustring& slot, const Meter& meter);

  void updateAccentButtons(const Meter& meter);

  void updateProfileList(const ProfileList& list);
  void updateProfileSelect(const Glib::ustring& id);
  void updateProfileTitle(const Glib::ustring& title, bool has_profile = true);
  void updateTempo(double tempo);
  void updateStart(bool running);

  void cancelButtonAnimations();

  void updateAccentAnimation(const audio::Ticker::Statistics& stats);
  void updateCurrentTempo(const audio::Ticker::Statistics& stats);
  void updatePendulum(const audio::Ticker::Statistics& stats);
  void onTickerStatistics(const audio::Ticker::Statistics& stats);
  void onTap(double confidence);

  void startTapAnimationTimer();
  void stopTapAnimationTimer();
  bool isTapAnimationTimerRunning();
  bool onTapAnimationTimer();

  void onMessage(const Message& message);
  void onMessageResponse(int response);

  // Settings
  void onSettingsPrefsChanged(const Glib::ustring& key);
  void updatePrefPendulumAction();
  void updatePrefPendulumPhaseMode();
  void updatePrefMeterAnimation();
  void updatePrefAnimationSync();
};

#endif//GMetronome_MainWindow_h
