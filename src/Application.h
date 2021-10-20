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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <gtkmm.h>
#include "ProfilesManager.h"
#include "Action.h"
#include "Ticker.h"

class MainWindow;

class Application : public Gtk::Application
{
public:
  Application();
  ~Application();
  
  static Glib::RefPtr<Application> create();
  
  sigc::signal<void, audio::Statistics> signal_ticker_statistics()
    { return signal_ticker_statistics_; }
  
protected:
  // Overrides of default signal handlers:
  void on_startup() override;
  void on_activate() override;
  
private:
  audio::Ticker ticker_;
  ProfilesManager profiles_manager_;
  
  // GSettings
  Glib::RefPtr<Gio::Settings> settings_;
  Glib::RefPtr<Gio::Settings> settings_prefs_;
  Glib::RefPtr<Gio::Settings> settings_state_;
  Glib::RefPtr<Gio::Settings> settings_shortcuts_;
  
  sigc::connection settings_prefs_connection_;
  sigc::connection settings_state_connection_;
  sigc::connection settings_shortcuts_connection_;
  
  sigc::connection timer_connection_;
  
  sigc::signal<void, audio::Statistics> signal_ticker_statistics_;

  // Main window
  MainWindow* main_window_;

  void initSettings();
  void initActions();
  void initProfiles();
  void initUI();
  void initTicker();
  
  void configureTickerSound();
  void configureTickerSoundHigh();
  void configureTickerSoundMid();
  void configureTickerSoundLow();
  
  Glib::RefPtr<Gio::SimpleAction> lookup_simple_action(const Glib::ustring& name);

  void setAccelerator(const ActionScope& scope,
                      const Glib::ustring& action_name,
                      const Glib::VariantBase& target_value,
                      const Glib::ustring& accel);
private:
  /*
   * signal and action handler
   */
  void onHideWindow(Gtk::Window* window);
  void onQuit(const Glib::VariantBase& parameter);

  // Volume
  void onVolumeIncrease(const Glib::VariantBase& value);
  void onVolumeDecrease(const Glib::VariantBase& value);

  // Tempo
  void onTempo(const Glib::VariantBase& value);
  void onTempoIncrease(const Glib::VariantBase& value);
  void onTempoDecrease(const Glib::VariantBase& value);
  void onTempoTap(const Glib::VariantBase& value);  
  
  // Meter
  void onMeterEnabled(const Glib::VariantBase& value);
  void onMeterSelect(const Glib::VariantBase& value);
  void onMeterChanged_1_Simple(const Glib::VariantBase& value);
  void onMeterChanged_2_Simple(const Glib::VariantBase& value);
  void onMeterChanged_3_Simple(const Glib::VariantBase& value);
  void onMeterChanged_4_Simple(const Glib::VariantBase& value);
  void onMeterChanged_1_Compound(const Glib::VariantBase& value);
  void onMeterChanged_2_Compound(const Glib::VariantBase& value);
  void onMeterChanged_3_Compound(const Glib::VariantBase& value);
  void onMeterChanged_4_Compound(const Glib::VariantBase& value);
  void onMeterChanged_Default(const Glib::ustring& action_name,
                               const Glib::VariantBase& value); 
  void onMeterChanged_Custom(const Glib::VariantBase& value);
  void onMeterChanged_SetState(const Glib::ustring& action_name,
                               Meter&& meter);
  // Trainer
  void onTrainerEnabled(const Glib::VariantBase& value);
  void onTrainerStart(const Glib::VariantBase& value);
  void onTrainerTarget(const Glib::VariantBase& value);
  void onTrainerAccel(const Glib::VariantBase& value);
  
  // Profiles
  void onProfilesManagerChanged();
  void onProfilesList(const Glib::VariantBase& value);
  void onProfilesSelect(const Glib::VariantBase& value);
  void convertActionToProfile(Profile::Content& content);
  void convertProfileToAction(const Profile::Content& content);
  void onProfilesNew(const Glib::VariantBase& value);
  void onProfilesDelete(const Glib::VariantBase& value);
  void onProfilesReset(const Glib::VariantBase& value);
  void onProfilesTitle(const Glib::VariantBase& value);
  void onProfilesDescription(const Glib::VariantBase& value);
  
  void loadSelectedProfile();
  void loadDefaultProfile();
  void saveSelectedProfile();
  
  // Transport and Volume
  void onStart(const Glib::VariantBase& value);
  //void onVolumeChanged();
  
  // Settings
  void onSettingsPrefsChanged(const Glib::ustring& key);
  void onSettingsStateChanged(const Glib::ustring& key);
  void onSettingsShortcutsChanged(const Glib::ustring& key);
  
  // Timer
  void startTimer();
  void stopTimer();
  bool onTimer();
};

#endif//__APPLICATION_H__
