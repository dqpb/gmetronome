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

#ifndef GMetronome_Application_h
#define GMetronome_Application_h

#include "ProfileManager.h"
#include "Action.h"
#include "Ticker.h"
#include "Message.h"

#include <gtkmm.h>
#include <bitset>

class MainWindow;

class Application : public Gtk::Application
{
public:
  Application();
  ~Application();

  static Glib::RefPtr<Application> create();

  sigc::signal<void, const Message&> signalMessage()
    { return signal_message_; }

  sigc::signal<void, const audio::Ticker::Statistics&> signalTickerStatistics()
    { return signal_ticker_statistics_; }

protected:
  // Overrides of default signal handlers:
  void on_startup() override;
  void on_activate() override;

private:
  audio::Ticker ticker_;
  ProfileManager profile_manager_;

  // Connections
  sigc::connection settings_state_connection_;
  sigc::connection settings_sound_theme_connection_;
  sigc::connection timer_connection_;

  // Signals
  sigc::signal<void, const Message&> signal_message_;
  sigc::signal<void, const audio::Ticker::Statistics&> signal_ticker_statistics_;

  // Main window
  MainWindow* main_window_;

  void initSettings();
  void initActions();
  void initProfiles();
  void initUI();
  void initTicker();

  using AccentMask = std::bitset<3>; // strong, mid, weak

  void configureTickerSound(const AccentMask& accents);
  void configureAudioBackend();
  void configureAudioDevice();

  Glib::RefPtr<Gio::SimpleAction> lookupSimpleAction(const Glib::ustring& name);

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
  void onMeterChanged_Simple2(const Glib::VariantBase& value);
  void onMeterChanged_Simple3(const Glib::VariantBase& value);
  void onMeterChanged_Simple4(const Glib::VariantBase& value);
  void onMeterChanged_Compound2(const Glib::VariantBase& value);
  void onMeterChanged_Compound3(const Glib::VariantBase& value);
  void onMeterChanged_Compound4(const Glib::VariantBase& value);
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
  void onProfileManagerChanged();
  void onProfileList(const Glib::VariantBase& value);
  void onProfileSelect(const Glib::VariantBase& value);
  void convertActionToProfile(Profile::Content& content);
  void convertProfileToAction(const Profile::Content& content);
  void onProfileNew(const Glib::VariantBase& value);
  void onProfileDelete(const Glib::VariantBase& value);
  void onProfileReset(const Glib::VariantBase& value);
  void onProfileTitle(const Glib::VariantBase& value);
  void onProfileDescription(const Glib::VariantBase& value);
  void onProfileReorder(const Glib::VariantBase& value);

  void loadSelectedProfile();
  void loadDefaultProfile();
  void saveSelectedProfile();

  // Transport and Volume
  void onStart(const Glib::VariantBase& value);

  // Audio Device
  void onAudioDeviceList(const Glib::VariantBase& value);
  Glib::ustring currentAudioDeviceKey();
  Glib::ustring currentAudioDevice();

  // Settings
  void onSettingsPrefsChanged(const Glib::ustring& key);
  void onSettingsStateChanged(const Glib::ustring& key);
  void onSettingsSoundChanged(const Glib::ustring& key);
  void onSettingsShortcutsChanged(const Glib::ustring& key);

  // Timer
  void startTimer();
  void stopTimer();
  bool onTimer();

  // Input validation
  std::pair<double,bool> validateTempo(double value);
  std::pair<double,bool> validateTrainerStart(double value);
  std::pair<double,bool> validateTrainerTarget(double value);
  std::pair<double,bool> validateTrainerAccel(double value);
  std::pair<double,bool> validateVolume(double value);
  std::pair<Meter,bool> validateMeter(Meter meter);
  std::pair<Glib::ustring,bool> validateMeterSlot(Glib::ustring str);
  std::pair<Glib::ustring,bool> validateProfileTitle(Glib::ustring str);
  std::pair<Glib::ustring,bool> validateProfileDescription(Glib::ustring str);
};

#endif//GMetronome_Application_h
