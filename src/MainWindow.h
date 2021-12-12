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

#ifndef GMetronome_MainWindow_h
#define GMetronome_MainWindow_h

#include "Action.h"
#include "Ticker.h"
#include "Message.h"
#include "AccentButtonGrid.h"

#include <gtkmm.h>
#include <list>
#include <vector>

class ActionBinding;
class ProfilesListStore;
class SettingsDialog;
class AccentButtonGrid;

class MainWindow : public Gtk::ApplicationWindow
{
public:
  MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainWindow();
  
  static MainWindow* create();

protected:
  Glib::RefPtr<Gtk::Builder> builder_;

  // GSettings
  Glib::RefPtr<Gio::Settings> settings_;
  Glib::RefPtr<Gio::Settings> settings_prefs_;
  Glib::RefPtr<Gio::Settings> settings_state_;
  Glib::RefPtr<Gio::Settings> settings_shortcuts_;

  //Bindings and Connections
  std::list<Glib::RefPtr<Glib::Binding>> bindings_;
  std::list<Glib::RefPtr<ActionBinding>> action_bindings_;
  std::vector<sigc::connection> meter_connections_;
  sigc::connection profiles_selection_changed_connection_;  
  sigc::connection profiles_title_start_editing_connection_;
  sigc::connection profiles_title_changed_connection_;
  sigc::connection profiles_popover_show_connection_;
  
  // Settings
  SettingsDialog* preferences_dialog_;

  // About
  Gtk::AboutDialog about_dialog_;

  // Shortcuts window
  Gtk::ShortcutsWindow* shortcuts_window_;
  
  // MainWindow UI elements
  class HeaderBarBin : public Gtk::Bin {} titlebar_bin_;
  Gtk::HeaderBar* header_bar_;
  Gtk::Label* tempo_integral_label_;
  Gtk::Label* tempo_fraction_label_;
  Gtk::Label* tempo_divider_label_;
  Gtk::Label* current_profile_label_;
  Gtk::Button* full_screen_button_;
  Gtk::Image* full_screen_image_;
  Gtk::MenuButton* menu_button_;
  Gtk::PopoverMenu* popover_menu_;
  Gtk::MenuButton* profiles_button_;
  Gtk::Popover* profiles_popover_;
  Gtk::TreeView* profiles_tree_view_;
  Gtk::Button* profiles_new_button_;
  Gtk::Button* profiles_delete_button_;
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
  Gtk::VolumeButton* volume_button_;
  Gtk::ToggleButton* start_button_;
  Gtk::Revealer* trainer_toggle_button_revealer_;
  Gtk::Revealer* accent_toggle_button_revealer_;
  Gtk::ToggleButton* trainer_toggle_button_;
  Gtk::ToggleButton* accent_toggle_button_;
  Gtk::Revealer* trainer_revealer_;
  Gtk::Revealer* accent_revealer_;
  Gtk::Frame* trainer_frame_;
  Gtk::Frame* accent_frame_;
  Gtk::Box* accent_box_;
  Gtk::Scale* tempo_scale_;
  Gtk::ComboBoxText* meter_combo_box_;
  Gtk::SpinButton* beats_spin_button_;
  Gtk::Label* beats_label_;
  Gtk::ComboBoxText* subdiv_combo_box_;
  Gtk::Label* subdiv_label_;

  AccentButtonGrid accent_button_grid_;

  Glib::RefPtr<Gtk::Adjustment> tempo_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_start_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_target_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> trainer_accel_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> beats_adjustment_;
  
  Glib::RefPtr<ProfilesListStore> profiles_list_store_;
  
  // cached preferences
  int meter_animation_;
  double animation_sync_usecs_;
  
  // Initialization
  void initSettings();
  void initActions();
  void initUI();
  void initAbout();
  void initBindings();

  // Default window signal handler
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
  void onMeterChanged();
  void onBeatsChanged();
  void onSubdivChanged();
  void onAccentChanged(std::size_t button_index);  
  void onProfilesSelectionChanged();
  void onProfilesTitleStartEditing(Gtk::CellEditable* editable, const Glib::ustring& path);
  void onProfilesTitleChanged(const Glib::ustring& path, const Glib::ustring& new_text);
  void onProfilesDragBegin(const Glib::RefPtr<Gdk::DragContext>& context);
  void onProfilesDragEnd(const Glib::RefPtr<Gdk::DragContext>& context);
  void onProfilesShow();
  void onProfilesHide();
  
  // Action handler
  void onActionStateChanged(const Glib::ustring& action_name,
			    const Glib::VariantBase& variant);

  void updateMeterInterface(const Glib::ustring& slot, const Meter& meter);
  
  void updateAccentButtons(const Meter& meter);
  void updateFlowBoxMaxChildren(int width);
  
  void updateProfilesList(const ProfilesList& list);
  void updateProfilesSelect(const Glib::ustring& id);
  void updateProfilesTitle(const Glib::ustring& title);
  void updateTempo(double tempo);
  void updateStart(bool running);

  void cancelButtonAnimations();

  void updateAccentAnimation(const audio::Ticker::Statistics& stats);
  void updateCurrentTempo(const audio::Ticker::Statistics& stats);
  void onTickerStatistics(const audio::Ticker::Statistics& stats);

  void onMessage(const Message& message);
  void onMessageResponse(int response);
  
  // Settings
  void onSettingsPrefsChanged(const Glib::ustring& key);
  void updatePrefAnimationSync();
  void updatePrefMeterAnimation();
};

#endif//GMetronome_MainWindow_h
