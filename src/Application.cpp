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
#include "MainWindow.h"
#include "ProfileIOLocalXml.h"
#include "Meter.h"
#include "Shortcut.h"
#include "Settings.h"

#include <chrono>
#include <cassert>
#include <algorithm>
#include <iostream>

Glib::RefPtr<Application> Application::create()
{
  return Glib::RefPtr<Application>(new Application());
}

Application::Application() : Gtk::Application(PACKAGE_ID)
{
  // nothing
}

Application::~Application()
{
  try {
    if (settings::sound() && settings::sound()->get_has_unapplied())
    {
      settings::sound()->apply();
      g_settings_sync();
    }
  }
  catch (...) {}

  try {
    if (settings::state())
      settings::state()->set_boolean(settings::kKeyStateFirstLaunch, false);
  }
  catch (...) {}
}

void Application::on_startup()
{
  //call the base class's implementation
  Gtk::Application::on_startup();

  auto desktop_id = Glib::ustring(PACKAGE_ID) + ".desktop";
  auto desktop_info = Gio::DesktopAppInfo::create(desktop_id);
  if (Glib::ustring appname = desktop_info ? desktop_info->get_locale_string("Name") : "";
      !appname.empty())
    Glib::set_application_name(appname);
  else
    Glib::set_application_name(PACKAGE_NAME);

  // initialize application settings (GSettings)
  initSettings();
  // initialize application actions
  initActions();
  // initialize UI (i.e.MainWindow)
  initUI();
  // initialize ticker
  initTicker();
  // initialize profile manager
  initProfiles();
}

void Application::on_activate()
{
  main_window_->present();
}

void Application::initSettings()
{
  settings::soundThemes()->settings()->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsSoundChanged));

  settings::preferences()->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsPrefsChanged));

  settings_state_connection_ = settings::state()->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsStateChanged));

  settings::sound()->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsSoundChanged));

  settings::shortcuts()->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsShortcutsChanged));

  // we cache sound prefs (e.g. volume adjustment) and propagate them
  // to the backend in the destructor
  settings::sound()->delay();
}

void Application::initActions()
{
  const ActionHandlerList kAppActionHandler =
    {
      {kActionQuit,            sigc::mem_fun(*this, &Application::onQuit)},

      {kActionVolume,          kActionNoSlot, settings::sound()},
      {kActionVolumeChange,    sigc::mem_fun(*this, &Application::onVolumeChange)},
      {kActionVolumeMute,      sigc::mem_fun(*this, &Application::onVolumeMute)},

      {kActionStart,           sigc::mem_fun(*this, &Application::onStart)},
      {kActionTempo,           sigc::mem_fun(*this, &Application::onTempo)},
      {kActionTempoChange,     sigc::mem_fun(*this, &Application::onTempoChange)},
      {kActionTempoScale,      sigc::mem_fun(*this, &Application::onTempoScale)},
      {kActionTempoTap,        sigc::mem_fun(*this, &Application::onTempoTap)},
      {kActionTrainerEnabled,  sigc::mem_fun(*this, &Application::onTrainerEnabled)},
      {kActionTrainerMode,     sigc::mem_fun(*this, &Application::onTrainerMode)},
      {kActionTrainerTarget,   sigc::mem_fun(*this, &Application::onTrainerTarget)},
      {kActionTrainerAccel,    sigc::mem_fun(*this, &Application::onTrainerAccel)},
      {kActionTrainerStep,     sigc::mem_fun(*this, &Application::onTrainerStep)},
      {kActionTrainerHold,     sigc::mem_fun(*this, &Application::onTrainerHold)},

      {kActionMeterEnabled,    sigc::mem_fun(*this, &Application::onMeterEnabled)},
      {kActionMeterSelect,     sigc::mem_fun(*this, &Application::onMeterSelect)},
      {kActionMeterSimple2,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple2)},
      {kActionMeterSimple3,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple3)},
      {kActionMeterSimple4,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple4)},
      {kActionMeterCompound2,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound2)},
      {kActionMeterCompound3,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound3)},
      {kActionMeterCompound4,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound4)},
      {kActionMeterCustom,     sigc::mem_fun(*this, &Application::onMeterChanged_Custom)},
      {kActionMeterSeek,       sigc::mem_fun(*this, &Application::onMeterSeek)},

      {kActionProfileList,         sigc::mem_fun(*this, &Application::onProfileList)},
      {kActionProfileSelect,       sigc::mem_fun(*this, &Application::onProfileSelect)},
      {kActionProfileNew,          sigc::mem_fun(*this, &Application::onProfileNew)},
      {kActionProfileDelete,       sigc::mem_fun(*this, &Application::onProfileDelete)},
      {kActionProfileReset,        sigc::mem_fun(*this, &Application::onProfileReset)},
      {kActionProfileTitle,        sigc::mem_fun(*this, &Application::onProfileTitle)},
      {kActionProfileDescription,  sigc::mem_fun(*this, &Application::onProfileDescription)},
      {kActionProfileReorder,      sigc::mem_fun(*this, &Application::onProfileReorder)},

      // The state of kActionAudioDeviceList provides a list of audio devices
      // as given by the current audio backend. It is not to be changed in
      // response of an "activation" or a "change_state" request of the client.
      {kActionAudioDeviceList,     kActionEmptySlot}
    };

  install_actions(*this, kAppActionHandler);
}

void Application::initUI()
{
  main_window_ = MainWindow::create();

  add_window(*main_window_);

  // Delete the window when it is hidden.
  main_window_->signal_hide()
    .connect(sigc::bind(sigc::mem_fun(*this, &Application::onHideWindow), main_window_));

  // initialize accelerators
  for (const auto& [key, shortcut_action] : kDefaultShortcutActionMap)
  {
    Glib::ustring accel = settings::shortcuts()->get_string(key);

    auto action = kActionDescriptions.find(shortcut_action.action_name);
    if (action != kActionDescriptions.end())
    {
      setAccelerator( action->second.scope,
                      shortcut_action.action_name,
                      shortcut_action.target_value,
                      accel );
    }
  }
}

void Application::initProfiles()
{
  profile_manager_.signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onProfileManagerChanged));

  profile_manager_.setIOModule(std::make_unique<ProfileIOLocalXml>());

  auto profile_list = profile_manager_.profileList();

  Glib::ustring restore_profile_id;
  if (settings::preferences()->get_boolean(settings::kKeyPrefsRestoreProfile))
  {
    restore_profile_id =
      restore_profile_id = settings::state()->get_string(settings::kKeyStateProfileSelect);
  }

  if (settings::state()->get_boolean(settings::kKeyStateFirstLaunch) && profile_list.empty())
  {
    Glib::Variant<Glib::ustring> state
      = Glib::Variant<Glib::ustring>::create(gettext(Profile::kDefaultTitle.c_str()));

    activate_action(kActionProfileNew, state);
  }
  else if (!restore_profile_id.empty())
  {
    Glib::Variant<Glib::ustring> state
      = Glib::Variant<Glib::ustring>::create(restore_profile_id);

    activate_action(kActionProfileSelect, state);
  }
  else
  {
    loadDefaultProfile();
  }
}

void Application::initTicker()
{
  loadSelectedSoundTheme();
  configureAudioBackend();
}

namespace {
  // helper
  std::string getErrorDetails(std::exception_ptr eptr)
  {
    std::string details;
    try
    {
      if (eptr) std::rethrow_exception(eptr);
    }
    catch(const audio::BackendError& e)
    {
      details += "Backend: ";
      switch(e.backend()) {
      case audio::BackendIdentifier::kNone: details += "none ("; break;
#if HAVE_ALSA
      case audio::BackendIdentifier::kALSA: details += "ALSA ("; break;
#endif
#if HAVE_OSS
      case audio::BackendIdentifier::kOSS: details += "OSS ("; break;
#endif
#if HAVE_PULSEAUDIO
      case audio::BackendIdentifier::kPulseAudio: details += "PulseAudio ("; break;
#endif
      default: details += "unknown ("; break;
      };

      switch(e.state()) {
      case audio::BackendState::kConfig: details += "config)"; break;
      case audio::BackendState::kOpen: details += "open)"; break;
      case audio::BackendState::kRunning: details += "running)"; break;
      default: details += "unknown) : "; break;
      };
      details += "\nWhat: ";
      details += e.what();
    }
    catch(const std::exception& e)
    {
      details += e.what();
    }
    return details;
  }
}//unnamed namespace

void Application::loadSelectedSoundTheme()
{
  std::for_each(settings_sound_params_connections_.begin(),
                settings_sound_params_connections_.end(),
                [] (auto& connection) { connection.disconnect(); });

  std::for_each(settings_sound_params_.begin(),
                settings_sound_params_.end(),
                [] (auto& settings) { settings.reset(); });

  if (auto theme_id = settings::soundThemes()->selected(); !theme_id.empty())
  {
    try {
      auto& theme_settings = settings::soundThemes()->settings(theme_id);

      for (auto accent : {kAccentWeak, kAccentMid, kAccentStrong})
      {
        const auto& path = settings::kSchemaPathSoundThemeParamsBasenameMap.at(accent);
        settings_sound_params_[accent] = theme_settings.children.at(path).settings;

        if (settings_sound_params_[accent])
        {
          settings_sound_params_connections_[accent] =
            settings_sound_params_[accent]->signal_changed().connect(
              [accent, this] (const Glib::ustring& key) {
                updateTickerSound(accent);
              });
        }
      }
    }
    catch(...) {
#ifndef NDEBUG
      std::cerr << "Application: failed to load sound theme '" << theme_id << "'" << std::endl;
#endif
    }
  }
  else {
#ifndef NDEBUG
      std::cerr << "Application: no sound theme selected" << std::endl;
#endif
  }

  updateTickerSound(kAccentMaskAll);
}

// Helper to compute the currently effective volume, i.e. it takes into account
// the current global volume, mute state and auto volume dropping when tapping.
double Application::getCurrentVolume() const
{
  if (queryVolumeMute())
    return 0.0;

  double global_volume = settings::sound()->get_double(settings::kKeySoundVolume);

  if (hasVolumeDrop())
    return std::clamp(
      global_volume - global_volume / 100.0 * getVolumeDrop(),
      0.0,
      100.0);
  else
    return global_volume;
}

void Application::updateTickerSound(Accent accent, double volume)
{
  if (accent == kAccentOff)
    return;

  if (volume <= 0.0)
    volume = getCurrentVolume();

  audio::SoundParameters params;

  if (settings_sound_params_[accent])
    SettingsListDelegate<SoundTheme>::loadParameters(settings_sound_params_[accent], params);

  params.volume *= volume / 100.0;
  ticker_.setSound(accent, params);
}

void Application::updateTickerSound(const AccentFlags& flags, double volume)
{
  if (flags.none())
    return;

  if (volume <= 0.0)
    volume = getCurrentVolume();

  for (auto accent : {kAccentWeak, kAccentMid, kAccentStrong})
    if (flags[accent])
      updateTickerSound(accent, volume);
}

void Application::configureAudioBackend()
{
  bool error = false;
  Message error_message;

  try {
    // dispose old backend and install a temporary dummy backend
    ticker_.getBackend();

    settings::AudioBackend backend = (settings::AudioBackend)
      settings::preferences()->get_enum(settings::kKeyPrefsAudioBackend);

    auto backend_id = settings::audioBackendToIdentifier(backend);

    // create new backend
    if (auto new_backend = audio::createBackend(backend_id);
        new_backend != nullptr)
    {
      // save device list
      auto audio_devices = new_backend->devices();

      std::vector<Glib::ustring> dev_list;
      dev_list.reserve(audio_devices.size());

      std::transform(audio_devices.begin(), audio_devices.end(), std::back_inserter(dev_list),
                     [] (const auto& dev) { return dev.name; });

      // configure and install new backend
      auto device_config = audio::kDefaultConfig;
      device_config.name = currentAudioDevice();

      new_backend->configure(device_config);
      ticker_.setBackend( std::move(new_backend) );

      // update device list action state
      Glib::Variant<std::vector<Glib::ustring>> dev_list_state
        = Glib::Variant<std::vector<Glib::ustring>>::create(dev_list);

      lookupSimpleAction(kActionAudioDeviceList)->set_state(dev_list_state);
    }
  }
  catch(const audio::BackendError& e)
  {
    error = true;
    error_message = getDefaultMessage(MessageIdentifier::kAudioError);
    error_message.details = getErrorDetails(std::current_exception());
  }
  catch(...)
  {
    error = true;
    error_message = getDefaultMessage(MessageIdentifier::kAudioError);
    error_message.details = getErrorDetails(std::current_exception());
  }

  if (error)
    signal_message_.emit(error_message);
}

void Application::configureAudioDevice()
{
  try {
    audio::DeviceConfig device_config =
    {
      currentAudioDevice(),
      audio::kDefaultSpec
    };

    if (auto backend = ticker_.getBackend(); backend)
    {
      backend->configure(device_config);
      ticker_.setBackend(std::move(backend));
    }
  }
  catch(...)
  {
    Message error_message = getDefaultMessage(MessageIdentifier::kAudioError);
    error_message.details = getErrorDetails(std::current_exception());
    signal_message_.emit(error_message);
  }
}

Glib::RefPtr<Gio::SimpleAction> Application::lookupSimpleAction(const Glib::ustring& name)
{
  Glib::RefPtr<Gio::Action> action = lookup_action(name);
  return Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(action);
}

void Application::setAccelerator(const ActionScope& scope,
                                 const Glib::ustring& action_name,
                                 const Glib::VariantBase& target_value,
                                 const Glib::ustring& accel)
{
  // validate accelerator
  guint accel_key;
  GdkModifierType accel_mods;

  gtk_accelerator_parse(accel.c_str(), &accel_key, &accel_mods);

  Glib::ustring prefix;

  switch (scope) {
  case ActionScope::kWin:
    prefix = "win.";
    break;
  case ActionScope::kApp:
  default:
    prefix = "app.";
  };

  Glib::ustring detailed_action_name
    = Gio::Action::print_detailed_name_variant( prefix + action_name, target_value );

  if ( accel_key != 0 || accel_mods != 0 )
    set_accel_for_action(detailed_action_name, accel);
  else
    unset_accels_for_action(detailed_action_name);
}

void Application::onHideWindow(Gtk::Window* window)
{
  saveSelectedProfile();

  if (queryStart())
    change_action_state(kActionStart, Glib::Variant<bool>::create(false));

  delete window;
}

void Application::onQuit(const Glib::VariantBase& parameter)
{
  auto windows = get_windows();

  // hide the window to call windows destr.
  for (auto window : windows)
    window->hide();

  // Not really necessary, when Gtk::Widget::hide() is called, unless
  // Gio::Application::hold() has been called without a corresponding call
  // to Gio::Application::release().
  quit();
}

void Application::onMeterEnabled(const Glib::VariantBase& value)
{
  Glib::Variant<bool> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);

  if (new_state.get())
    ticker_.setMeter(queryMeter(queryMeterSelect()));
  else
    ticker_.resetMeter();

  lookupSimpleAction(kActionMeterEnabled)->set_state(new_state);
}

void Application::onMeterSelect(const Glib::VariantBase& value)
{
  auto in_slot =
    Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

  if (auto old_slot = queryMeterSelect(); in_slot != old_slot)
  {
    if (auto [new_slot, valid] = validateMeterSlot(std::move(in_slot)); valid)
    {
      if (queryMeterEnabled())
        ticker_.setMeter(queryMeter(new_slot));

      Glib::Variant<Glib::ustring> out_state =
        Glib::Variant<Glib::ustring>::create(new_slot);

      lookupSimpleAction(kActionMeterSelect)->set_state(out_state);
    }
  }
}

void Application::onMeterChanged_Simple2(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterSimple2, value); }

void Application::onMeterChanged_Simple3(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterSimple3, value); }

void Application::onMeterChanged_Simple4(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterSimple4, value); }

void Application::onMeterChanged_Compound2(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterCompound2, value); }

void Application::onMeterChanged_Compound3(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterCompound3, value); }

void Application::onMeterChanged_Compound4(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeterCompound4, value); }

void Application::onMeterChanged_Default(const Glib::ustring& action_name,
                                         const Glib::VariantBase& value)
{
  Glib::Variant<Meter> in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Meter>>(value);

  Meter new_meter = in_state.get();
  Meter old_meter = queryMeter(action_name);

  if (old_meter.beats() == new_meter.beats()
      && old_meter.division() == new_meter.division())
  {
    onMeterChanged_SetState(action_name, std::move(new_meter));
  }
}

void Application::onMeterChanged_Custom(const Glib::VariantBase& value)
{
  Glib::Variant<Meter> in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Meter>>(value);

  Meter meter = in_state.get();

  onMeterChanged_SetState(kActionMeterCustom, std::move(meter));
}

void Application::onMeterChanged_SetState(const Glib::ustring& action_name, Meter&& in_meter)
{
  if (auto action = lookupSimpleAction(action_name); action)
  {
    auto [meter, valid] = validateMeter(in_meter);

    // keep this line before ticker_.setMeter call (meter is moved)
    Glib::Variant<Meter> out_state = Glib::Variant<Meter>::create(meter);

    if (queryMeterEnabled() && queryMeterSelect() == action_name)
      ticker_.setMeter(std::move(meter));

    action->set_state(out_state);
  }
}

void Application::onMeterSeek(const Glib::VariantBase& value)
{
  // not implemented yet
}

void Application::onVolumeChange(const Glib::VariantBase& value)
{
  double delta_volume =
    Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double current_volume = settings::sound()->get_double(settings::kKeySoundVolume);

  auto [new_volume, valid] = validateVolume(current_volume + delta_volume);

  settings::sound()->set_double(settings::kKeySoundVolume, new_volume);
}

void Application::onVolumeMute(const Glib::VariantBase& value)
{
  lookupSimpleAction(kActionVolumeMute)->set_state(value);
  updateTickerSound(kAccentMaskAll);
}

double Application::getReferenceTempo() const
{
  double ref_tempo = 0.0;

  if (audio::Ticker::State state = ticker_.state();
      state.test(audio::Ticker::StateFlag::kStarted)
      && !state.test(audio::Ticker::StateFlag::kError))
  {
    const auto& stats = ticker_.getStatistics();

    if (stats.generator == audio::kRegularGenerator)
    {
      if (stats.syncing)
        ref_tempo = -1.0;
      else
        ref_tempo = stats.tempo;
    }
  }
  if (ref_tempo == 0.0)
    ref_tempo =  queryTempo();

  return ref_tempo;
}

void Application::onTempo(const Glib::VariantBase& value)
{
  double in_tempo =
    Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  auto [tempo, valid] = validateTempo(in_tempo);

  ticker_.setTempo(tempo);

  auto new_tempo_state = Glib::Variant<double>::create(tempo);
  lookupSimpleAction(kActionTempo)->set_state(new_tempo_state);
}

void Application::onTempoChange(const Glib::VariantBase& value)
{
  double delta_tempo =
    Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  if (delta_tempo == 0.0)
    return;

  double tempo = getReferenceTempo();
  if (tempo <= 0.0)
    return;

  double integral;
  double fractional = std::modf(tempo, &integral);

  // we just round up/down if the change value is one/minus one respectively
  if (std::abs(delta_tempo) == 1.0 && fractional != 0.0)
  {
    if (delta_tempo < 0.0)
      tempo = integral;
    else
      tempo = std::round(integral + 1.0);
  }
  else
    tempo += delta_tempo;

  auto new_tempo_state = Glib::Variant<double>::create(tempo);
  activate_action(kActionTempo, new_tempo_state);
}

void Application::onTempoScale(const Glib::VariantBase& value)
{
  double scale = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double tempo = getReferenceTempo();
  if (tempo <= 0.0)
    return;

  double new_tempo = scale * tempo;

  auto new_tempo_state = Glib::Variant<double>::create(new_tempo);
  activate_action(kActionTempo, new_tempo_state);
}

namespace {

  using std::chrono::milliseconds;
  using std::literals::chrono_literals::operator""ms;

  constexpr double        kMaxVolumeDrop            = 85.0;   // percent
  constexpr milliseconds  kDropVolumeTimerInterval  = 250ms;
  constexpr double        kDropVolumeRecoverSpeed   = 30.0;   // percent/s
  constexpr milliseconds  kSingleTapSyncTime        = 1000ms;

}//unnamed namespace

void Application::onTempoTap(const Glib::VariantBase& value)
{
  auto ticker_stats = ticker_.getStatistics(false);

  double new_tempo = ticker_stats.tempo;

  milliseconds sync_time = kSingleTapSyncTime;

  const auto& [tap, estimate] = tap_analyser_.tap(1.0);
  const auto& [tempo, phase, confidence] = estimate;

  if (tap.flags.test(TapAnalyser::kValid) && !tap.flags.test(TapAnalyser::kInit))
  {
    new_tempo = std::clamp(tempo, Profile::kMinTempo, Profile::kMaxTempo);

    if (settings::preferences()->get_boolean(settings::kKeyPrefsRoundTappedTempo))
      new_tempo = std::round(new_tempo);

    sync_time = milliseconds(
      static_cast<milliseconds::rep>(std::ceil(60.0 * 1000.0 / new_tempo)));

    Glib::Variant<double> new_tempo_state = Glib::Variant<double>::create( new_tempo );
    activate_action(kActionTempo, new_tempo_state);
  }

  if (audio::Ticker::State state = ticker_.state();
      state.test(audio::Ticker::StateFlag::kStarted))
  {
    if (settings::sound()->get_boolean(settings::kKeySoundAutoAdjustVolume))
        startDropVolumeTimer( (1.0 - confidence) * kMaxVolumeDrop );
  }

  // Synchronize the tap with the nearest audible beat:
  //
  //      +--latency-+         stats
  //      |          |           |
  //   ---1----------:---2-------:----:-3-------> device beat
  //   --------------1-----------:--2-:---------> audible beat (device + latency)
  //   --s-------s-------s-------s----:--s------> device stats creation (time stamp)
  //   ->>>--time-->>>----------------|--------->
  //                                 now (tap time)

  // tempo in beats per microsecond
  double current_tempo_bpus = ticker_stats.tempo / 60.0 / 1000000.0;
  double new_tempo_bpus = new_tempo / 60.0 / 1000000.0;

  // device beat at tap time
  double device_beat = ticker_stats.position
    + current_tempo_bpus * (tap.time - ticker_stats.timestamp).count();

  // audible beat at tap time
  double audible_beat = device_beat
    - current_tempo_bpus * ticker_stats.backend_latency.count();

  double beat_dev = std::round(audible_beat) - audible_beat
    + new_tempo_bpus * (phase - tap.time).count(); // deviation from the
                                                   // estimated tap time (phase)

  // If the beat deviation is large compared to the sync time, the synchronize function
  // currently tends to produce very low (even negative) and very high intermediate tempos.
  // To even this a bit, we add some time in the magnitude of the deviation.
  sync_time += milliseconds(static_cast<milliseconds::rep>(std::abs(beat_dev) * 1000.0));

  // But we limit the sync time to the time of two consecutive beats at the lowest
  // possible tempo; this prevents a case, where two Ticker::synchronize() calls
  // without and intermediate Ticker::setTempo() call (caused by a timeout in the
  // tap analyser) lead to a drifting tempo in the generator.
  sync_time = std::min(
    sync_time, milliseconds(static_cast<milliseconds::rep>(60.0 * 1000.0 / Profile::kMinTempo)));

  // apply phase adjustment
  ticker_.synchronize(beat_dev, 0.0, sync_time);

  signal_tap_.emit(confidence);
}

void Application::configureTickerForTrainerMode(Profile::TrainerMode mode)
{
  switch (mode) {

  case Profile::TrainerMode::kContinuous: {
    auto [accel, target] = queryTrainerContinuousParams();
    ticker_.accelerate(accel, target);
  }
  break;

  case Profile::TrainerMode::kStepwise: {
    auto [hold, step, target] = queryTrainerStepwiseParams();
    ticker_.accelerate(hold, step, target);
  }
  break;
  };
}

void Application::onTrainerEnabled(const Glib::VariantBase& value)
{
  bool in_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value).get();

  if (in_enabled)
    configureTickerForTrainerMode(queryTrainerMode());
  else
    ticker_.stopAcceleration();

  lookupSimpleAction(kActionTrainerEnabled)->set_state(value);
}

void Application::onTrainerMode(const Glib::VariantBase& value)
{
  Profile::TrainerMode in_mode =
    Glib::VariantBase::cast_dynamic<Glib::Variant<Profile::TrainerMode>>(value).get();

  if (auto [mode, valid] = validateTrainerMode(in_mode); valid)
  {
    if (queryTrainerEnabled())
      configureTickerForTrainerMode(in_mode);

    auto new_state = Glib::Variant<Profile::TrainerMode>::create(mode);
    lookupSimpleAction(kActionTrainerMode)->set_state(new_state);
  }
}

void Application::onTrainerTarget(const Glib::VariantBase& value)
{
  double in_target = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [target, valid] = validateTrainerTarget(in_target);

  if (queryTrainerEnabled())
  {
    Profile::TrainerMode mode = queryTrainerMode();

    if (mode == Profile::TrainerMode::kContinuous)
      ticker_.accelerate(queryTrainerAccel(), target);
    else if (mode == Profile::TrainerMode::kStepwise)
      ticker_.accelerate(queryTrainerHold(), queryTrainerStep(), target);
  }
  auto new_state = Glib::Variant<double>::create(target);
  lookupSimpleAction(kActionTrainerTarget)->set_state(new_state);
}

void Application::onTrainerAccel(const Glib::VariantBase& value)
{
  double in_accel = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [accel, valid] = validateTrainerAccel(in_accel);

  if (queryTrainerEnabled())
  {
    Profile::TrainerMode mode = queryTrainerMode();

    if (mode == Profile::TrainerMode::kContinuous)
      ticker_.accelerate(accel, queryTrainerTarget());
  }
  auto new_state = Glib::Variant<double>::create(accel);
  lookupSimpleAction(kActionTrainerAccel)->set_state(new_state);
}

void Application::onTrainerStep(const Glib::VariantBase& value)
{
  double in_step = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [step, valid] = validateTrainerStep(in_step);

  if (queryTrainerEnabled())
  {
    Profile::TrainerMode mode = queryTrainerMode();

    if (mode == Profile::TrainerMode::kStepwise)
      ticker_.accelerate(queryTrainerHold(), step, queryTrainerTarget());
  }
  auto new_state = Glib::Variant<double>::create(step);
  lookupSimpleAction(kActionTrainerStep)->set_state(new_state);
}

void Application::onTrainerHold(const Glib::VariantBase& value)
{
  int in_hold = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(value).get();
  auto [hold, valid] = validateTrainerHold(in_hold);

  if (queryTrainerEnabled())
  {
    Profile::TrainerMode mode = queryTrainerMode();

    if (mode == Profile::TrainerMode::kStepwise)
      ticker_.accelerate(hold, queryTrainerStep(), queryTrainerTarget());
  }
  auto new_state = Glib::Variant<int>::create(hold);
  lookupSimpleAction(kActionTrainerHold)->set_state(new_state);
}

void Application::onProfileManagerChanged()
{
  auto in_list = profile_manager_.profileList();

  ProfileList out_list;
  std::transform(in_list.begin(), in_list.end(), std::back_inserter(out_list),
                 [] (const auto& primer) -> ProfileListEntry {
                   return {primer.id, primer.header.title, primer.header.description};
                 });

  Glib::Variant<ProfileList> out_list_state
    = Glib::Variant<ProfileList>::create(out_list);

  lookupSimpleAction(kActionProfileList)->set_state(out_list_state);

  // update selection
  if (Glib::ustring selected_id = queryProfileSelect(); !selected_id.empty())
  {
    auto it = std::find_if(out_list.begin(), out_list.end(),
                           [&selected_id] (const auto& p) -> bool {
                             const auto& id = std::get<kProfileListEntryIdentifier>(p);
                             return id == selected_id;
                           });

    if (it != out_list.end())
    {
      Glib::Variant<Glib::ustring> title_state
        = Glib::Variant<Glib::ustring>::create(std::get<kProfileListEntryTitle>(*it));

      Glib::Variant<Glib::ustring> descr_state
        = Glib::Variant<Glib::ustring>::create(std::get<kProfileListEntryDescription>(*it));

      lookupSimpleAction(kActionProfileTitle)->set_state(title_state);
      lookupSimpleAction(kActionProfileDescription)->set_state(descr_state);
    }
    else
    {
      Glib::Variant<Glib::ustring> empty_state
        = Glib::Variant<Glib::ustring>::create({""});

      activate_action(kActionProfileSelect, empty_state);
    }
  }
}

void Application::onProfileList(const Glib::VariantBase& value)
{
  // The "profile-list" action state is modified in response to "signal_changed"
  // of the profile manager. (see onProfileManagerChanged() signal handler)
  // It gives clients access to an up-to-date list of all available profiles
  // but can not be modified by clients via activate_action() or change_action_state().
}

void Application::onProfileSelect(const Glib::VariantBase& value)
{
  saveSelectedProfile();

  const Glib::Variant<Glib::ustring> in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

  const Glib::Variant<Glib::ustring> empty_string = Glib::Variant<Glib::ustring>::create({""});

  Glib::Variant<Glib::ustring> out_state = empty_string;
  Glib::Variant<Glib::ustring> profile_title = empty_string;
  Glib::Variant<Glib::ustring> profile_description = empty_string;

  if ( ! in_state.get().empty() )
  {
    ProfileList plist = queryProfileList();

    if ( auto it = std::find_if(plist.begin(), plist.end(),
                                [&in_state] (auto& e) {
                                  return std::get<kProfileListEntryIdentifier>(e) == in_state.get();
                                });
         it != plist.end())
    {
      out_state = in_state;

      const Glib::ustring& title = std::get<kProfileListEntryTitle>(*it);
      const Glib::ustring& description = std::get<kProfileListEntryDescription>(*it);

      profile_title = Glib::Variant<Glib::ustring>::create( title );
      profile_description = Glib::Variant<Glib::ustring>::create( description );
    }
  }

  lookupSimpleAction(kActionProfileSelect)->set_state(out_state);
  lookupSimpleAction(kActionProfileTitle)->set_state(profile_title);
  lookupSimpleAction(kActionProfileDescription)->set_state(profile_description);

  settings_state_connection_.block();
  settings::state()->set_string(settings::kKeyStateProfileSelect, out_state.get());
  settings_state_connection_.unblock();

  loadSelectedProfile();
}

void Application::convertActionToProfile(Profile::Content& content)
{
  get_action_state(kActionTempo, content.tempo);
  get_action_state(kActionMeterEnabled, content.meter_enabled);

  Glib::ustring tmp_string = queryMeterSelect();
  content.meter_select = tmp_string.raw();

  get_action_state(kActionMeterSimple2, content.meter_simple_2);
  get_action_state(kActionMeterSimple3, content.meter_simple_3);
  get_action_state(kActionMeterSimple4, content.meter_simple_4);
  get_action_state(kActionMeterCompound2, content.meter_compound_2);
  get_action_state(kActionMeterCompound3, content.meter_compound_3);
  get_action_state(kActionMeterCompound4, content.meter_compound_4);
  get_action_state(kActionMeterCustom, content.meter_custom);
  get_action_state(kActionTrainerEnabled, content.trainer_enabled);
  get_action_state(kActionTrainerMode, content.trainer_mode);
  get_action_state(kActionTrainerTarget, content.trainer_target);
  get_action_state(kActionTrainerAccel, content.trainer_accel);
  get_action_state(kActionTrainerStep, content.trainer_step);
  get_action_state(kActionTrainerHold, content.trainer_hold);

  if (settings::preferences()->get_boolean(settings::kKeyPrefsLinkSoundTheme))
    content.sound_theme_id = settings::soundThemes()->selected();
}

void Application::convertProfileToAction(const Profile::Content& content)
{
  activate_action(kActionTempo,
                  Glib::Variant<double>::create(content.tempo) );
  change_action_state(kActionMeterEnabled,
                      Glib::Variant<bool>::create(content.meter_enabled) );
  activate_action(kActionMeterSelect,
                  Glib::Variant<Glib::ustring>::create(content.meter_select) );
  activate_action(kActionMeterSimple2,
                  Glib::Variant<Meter>::create(content.meter_simple_2) );
  activate_action(kActionMeterSimple3,
                  Glib::Variant<Meter>::create(content.meter_simple_3) );
  activate_action(kActionMeterSimple4,
                  Glib::Variant<Meter>::create(content.meter_simple_4) );
  activate_action(kActionMeterCompound2,
                  Glib::Variant<Meter>::create(content.meter_compound_2) );
  activate_action(kActionMeterCompound3,
                  Glib::Variant<Meter>::create(content.meter_compound_3) );
  activate_action(kActionMeterCompound4,
                  Glib::Variant<Meter>::create(content.meter_compound_4) );
  activate_action(kActionMeterCustom,
                  Glib::Variant<Meter>::create(content.meter_custom) );
  change_action_state(kActionTrainerEnabled,
                      Glib::Variant<bool>::create(content.trainer_enabled) );
  activate_action(kActionTrainerMode,
                  Glib::Variant<Profile::TrainerMode>::create(content.trainer_mode) );
  activate_action(kActionTrainerTarget,
                  Glib::Variant<double>::create(content.trainer_target) );
  activate_action(kActionTrainerAccel,
                  Glib::Variant<double>::create(content.trainer_accel) );
  activate_action(kActionTrainerStep,
                  Glib::Variant<double>::create(content.trainer_step) );
  activate_action(kActionTrainerHold,
                  Glib::Variant<int>::create(content.trainer_hold) );

  if (settings::preferences()->get_boolean(settings::kKeyPrefsLinkSoundTheme))
  {
    if (content.sound_theme_id.empty()
        || !settings::soundThemes()->select(content.sound_theme_id))
    {
      if (auto theme_list_settings = settings::soundThemes()->settings(); theme_list_settings)
        theme_list_settings->reset(settings::kKeySettingsListSelectedEntry);
    }
  }
}

void Application::onProfileNew(const Glib::VariantBase& value)
{
  Glib::Variant<Glib::ustring> in_title
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

  auto [title,valid] = validateProfileTitle(in_title.get());

  Profile::Header header = {title, ""};
  Profile::Content content;

  convertActionToProfile(content);

  Profile::Primer primer = profile_manager_.newProfile(header, content);

  auto id = Glib::Variant<Glib::ustring>::create(primer.id);

  activate_action(kActionProfileSelect, id);
}

void Application::loadSelectedProfile()
{
  Glib::ustring id = queryProfileSelect();
  bool has_selected_id = !id.empty();

  if (has_selected_id)
  {
    Profile::Content content = profile_manager_.getProfileContent(id);
    convertProfileToAction(content);
  }
  lookupSimpleAction(kActionProfileDelete)->set_enabled(has_selected_id);
  lookupSimpleAction(kActionProfileTitle)->set_enabled(has_selected_id);
  lookupSimpleAction(kActionProfileDescription)->set_enabled(has_selected_id);
}

void Application::loadDefaultProfile()
{
  Glib::ustring id = queryProfileSelect();
  bool has_selected_id = !id.empty();

  convertProfileToAction(kDefaultProfile.content);

  lookupSimpleAction(kActionProfileDelete)->set_enabled(has_selected_id);
  lookupSimpleAction(kActionProfileTitle)->set_enabled(has_selected_id);
  lookupSimpleAction(kActionProfileDescription)->set_enabled(has_selected_id);
}

void Application::saveSelectedProfile()
{
  if (Glib::ustring id = queryProfileSelect(); !id.empty())
  {
    Profile::Content content = profile_manager_.getProfileContent(id);
    convertActionToProfile(content);
    profile_manager_.setProfileContent(id, content);
  }
}

void Application::onProfileDelete(const Glib::VariantBase& value)
{
  if (Glib::ustring id = queryProfileSelect(); !id.empty())
  {
    Glib::Variant<Glib::ustring> empty_state
      = Glib::Variant<Glib::ustring>::create({""});

    activate_action(kActionProfileSelect, empty_state);

    profile_manager_.deleteProfile(id);
  }
}

void Application::onProfileReset(const Glib::VariantBase& value)
{
  loadDefaultProfile();
}

void Application::onProfileTitle(const Glib::VariantBase& value)
{
  if (Glib::ustring id = queryProfileSelect(); !id.empty())
  {
    Glib::Variant<Glib::ustring> in_value
      = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    auto [title, valid] = validateProfileTitle(in_value.get());

    Glib::Variant<Glib::ustring> out_value
      = Glib::Variant<Glib::ustring>::create(title);

    Profile::Header header = profile_manager_.getProfileHeader(id);
    header.title = out_value.get();

    profile_manager_.setProfileHeader(id, header);

    lookupSimpleAction(kActionProfileTitle)->set_state(out_value);
  }
}

void Application::onProfileDescription(const Glib::VariantBase& value)
{
  if (Glib::ustring id = queryProfileSelect(); !id.empty())
  {
    Glib::Variant<Glib::ustring> in_value
      = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    auto [descr, valid] = validateProfileDescription(in_value.get());

    Glib::Variant<Glib::ustring> out_value
      = Glib::Variant<Glib::ustring>::create(descr);

    Profile::Header header = profile_manager_.getProfileHeader(id);
    header.description = out_value.get();

    profile_manager_.setProfileHeader(id, header);

    lookupSimpleAction(kActionProfileDescription)->set_state(out_value);
  }
}

void Application::onProfileReorder(const Glib::VariantBase& value)
{
  Glib::Variant<ProfileIdentifierList> in_list
    = Glib::VariantBase::cast_dynamic<Glib::Variant<ProfileIdentifierList>>(value);

  std::vector<Profile::Identifier> out_list;

  // convert ProfileIdentifierList to std::vector<Profile::Identifier>
  auto iter = in_list.get_iter();
  out_list.reserve(in_list.get_n_children());

  Glib::Variant<ProfileIdentifierList::value_type> id;
  while (iter.next_value(id))
      out_list.push_back(id.get());

  profile_manager_.reorderProfiles(out_list);
}

void Application::onStart(const Glib::VariantBase& value)
{
  Glib::Variant<bool> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);

  bool error = false;
  Message error_message;

  try {
    if (new_state.get())
    {
      ticker_.start();
      startStatsTimer();
    }
    else
    {
      stopStatsTimer();
      ticker_.stop();
    }
  }
  catch(const audio::BackendError& e)
  {
    error = true;
    error_message = getDefaultMessage(MessageIdentifier::kAudioError);
    error_message.details = getErrorDetails(std::current_exception());
  }
  catch(const GMetronomeError& e)
  {
    error = true;
    error_message = getDefaultMessage(MessageIdentifier::kAudioError);
    error_message.details = getErrorDetails(std::current_exception());
  }
  catch(...)
  {
    error = true;
    error_message = getDefaultMessage(MessageIdentifier::kGenericError);
    error_message.details = getErrorDetails(std::current_exception());
  }

  if (error)
  {
    ticker_.reset();
    new_state = Glib::Variant<bool>::create(false);
    signal_message_.emit(error_message);
  }

  lookupSimpleAction(kActionStart)->set_state(new_state);
}

Glib::ustring Application::currentAudioDeviceKey()
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings::preferences()->get_enum(settings::kKeyPrefsAudioBackend);

  if (auto it = settings::kBackendToDeviceMap.find(backend);
      it != settings::kBackendToDeviceMap.end())
  {
    return it->second;
  }
  else return "";
}

Glib::ustring Application::currentAudioDevice()
{
  if (auto key = currentAudioDeviceKey(); !key.empty())
    return settings::preferences()->get_string(key);
  else
    return "";
}

void Application::onSettingsPrefsChanged(const Glib::ustring& key)
{
  if (key == settings::kKeyPrefsLinkSoundTheme)
  {
    // load sound theme from selected profile
    if (settings::preferences()->get_boolean(settings::kKeyPrefsLinkSoundTheme))
    {
      if (Glib::ustring id = queryProfileSelect(); !id.empty())
      {
        if (Profile::Content content = profile_manager_.getProfileContent(id);
            !content.sound_theme_id.empty())
          settings::soundThemes()->select(content.sound_theme_id);
      }
    }
  }
  if (key == settings::kKeyPrefsAudioBackend)
  {
    configureAudioBackend();
  }
  else if (key == currentAudioDeviceKey())
  {
    configureAudioDevice();
  }
}

void Application::onSettingsStateChanged(const Glib::ustring& key)
{
  if (key == settings::kKeyStateProfileSelect)
  {
    Glib::ustring profile_id = settings::state()->get_string(settings::kKeyStateProfileSelect);

    Glib::Variant<Glib::ustring> state
      = Glib::Variant<Glib::ustring>::create(profile_id);

    activate_action(kActionProfileSelect, state);
  }
}

void Application::onSettingsSoundChanged(const Glib::ustring& key)
{
  if (key == settings::kKeySoundVolume)
  {
    if (queryVolumeMute())
      activate_action(kActionVolumeMute);
    else
      updateTickerSound(kAccentMaskAll);
  }
  else if (key == settings::kKeySettingsListSelectedEntry)
  {
    // store sound theme to selected profile
    if (settings::preferences()->get_boolean(settings::kKeyPrefsLinkSoundTheme))
    {
      if (Glib::ustring id = queryProfileSelect(); !id.empty())
      {
        Profile::Content content = profile_manager_.getProfileContent(id);
        content.sound_theme_id = settings::soundThemes()->selected();
        profile_manager_.setProfileContent(id, content);
      }
    }
    loadSelectedSoundTheme();
  }
  else if (key == settings::kKeySettingsListEntries)
  {
    /* nothing */
  }
}

void Application::onSettingsShortcutsChanged(const Glib::ustring& key)
{
  auto shortcut_action = kDefaultShortcutActionMap.find(key);
  if (shortcut_action != kDefaultShortcutActionMap.end())
  {
    Glib::ustring accel = settings::shortcuts()->get_string(key);

    auto action = kActionDescriptions.find(shortcut_action->second.action_name);
    if (action != kActionDescriptions.end())
    {
      setAccelerator( action->second.scope,
                      shortcut_action->second.action_name,
                      shortcut_action->second.target_value,
                      accel );
    }
  }
}

namespace {
  constexpr milliseconds kUpdateStatsTimerInterval = 60ms;
}

void Application::startStatsTimer()
{
  stats_timer_connection_ = Glib::signal_timeout()
    .connect(sigc::mem_fun(*this, &Application::onStatsTimer),
             kUpdateStatsTimerInterval.count());
}

void Application::stopStatsTimer()
{
  stats_timer_connection_.disconnect();
  signal_ticker_statistics_.emit(audio::Ticker::Statistics{});
}

bool Application::onStatsTimer()
{
  if (audio::Ticker::State state = ticker_.state();
      state.test(audio::Ticker::StateFlag::kError))
  {
    // this will handle the error
    change_action_state(kActionStart, Glib::Variant<bool>::create(false));
    return false;
  }
  else
  {
    if (ticker_.hasStatistics())
    {
      audio::Ticker::Statistics stats = ticker_.getStatistics(true);
      signal_ticker_statistics_.emit(stats);
    }
    return true;
  }
}

void Application::startDropVolumeTimer(double drop)
{
  setVolumeDrop(std::max(getVolumeDrop(), std::clamp(drop, 0.0, 100.0)));

  if (!isDropVolumeTimerRunning())
  {
    volume_timer_connection_ = Glib::signal_timeout()
      .connect(sigc::mem_fun(*this, &Application::onDropVolumeTimer),
               kDropVolumeTimerInterval.count());
  }
}

void Application::stopDropVolumeTimer()
{
  if (isDropVolumeTimerRunning())
  {
    volume_timer_connection_.disconnect();
    setVolumeDrop(0.0);
  }
}

bool Application::isDropVolumeTimerRunning()
{
  return volume_timer_connection_.connected();
}

bool Application::onDropVolumeTimer()
{
  constexpr double kVolumeRecover = kDropVolumeRecoverSpeed * kDropVolumeTimerInterval / 1000ms;
  setVolumeDrop(std::clamp(getVolumeDrop() - kVolumeRecover, 0.0, 100.0));
  return hasVolumeDrop();
}

void Application::setVolumeDrop(double drop)
{
  volume_drop_ = std::clamp(drop, 0.0, 100.0);
  updateTickerSound(kAccentMaskAll);
}

std::pair<double,bool> Application::validateTempo(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTempo, range);
  return validateActionState(value, range);
}

std::pair<Profile::TrainerMode,bool> Application::validateTrainerMode(Profile::TrainerMode value)
{
  ActionStateHintArray<Profile::TrainerMode> allowed_values;
  get_action_state_hint(kActionTrainerMode, allowed_values);
  return validateActionState(value, allowed_values);
}

std::pair<double,bool> Application::validateTrainerTarget(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerTarget, range);
  return validateActionState(value, range);
}

std::pair<double,bool> Application::validateTrainerAccel(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerAccel, range);
  return validateActionState(value, range);
}

std::pair<double,bool> Application::validateTrainerStep(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerStep, range);
  return validateActionState(value, range);
}

std::pair<int,bool> Application::validateTrainerHold(int value)
{
  ActionStateHintRange<int> range;
  get_action_state_hint(kActionTrainerHold, range);
  return validateActionState(value, range);
}

std::pair<double,bool> Application::validateVolume(double value)
{
  return validateActionStateRange(value, settings::kMinVolume, settings::kMaxVolume);
}

std::pair<Meter,bool> Application::validateMeter(Meter meter)
{
  // nothing to do since a constructed meter object is always valid
  return {meter, true};
}

std::pair<Glib::ustring,bool> Application::validateMeterSlot(Glib::ustring str)
{
  if (str == kActionMeterSimple2
      || str == kActionMeterSimple3
      || str == kActionMeterSimple4
      || str == kActionMeterCompound2
      || str == kActionMeterCompound3
      || str == kActionMeterCompound4
      || str == kActionMeterCustom)
  {
    return {str, true};
  }
  else
    return {queryMeterSelect(), false};
}

//helper
std::pair<Glib::ustring,bool> validateUTF8String(Glib::ustring str, int maxLength = -1)
{
  Glib::ustring ret;

#if GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 62
  ret = str.make_valid();
#else
  if (str.validate())
    ret = str;
  else
    ret = "";
#endif

  if (maxLength >= 0)
    ret = ret.substr(0, maxLength);

  return {ret, ret.raw() == str.raw()};
}

std::pair<Glib::ustring,bool> Application::validateProfileTitle(Glib::ustring str)
{ return validateUTF8String(str, Profile::kTitleMaxLength); }

std::pair<Glib::ustring,bool> Application::validateProfileDescription(Glib::ustring str)
{ return validateUTF8String(str, Profile::kDescriptionMaxLength); }
