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

#include "Application.h"
#include "ProfilesIOLocalXml.h"
#include "MainWindow.h"
#include "Meter.h"
#include "Shortcut.h"
#include "Settings.h"

#include <cassert>
#include <iostream>

namespace {

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
      case settings::kAudioBackendNone: details += "none ("; break;
#if HAVE_ALSA
      case settings::kAudioBackendAlsa: details += "alsa ("; break;
#endif
#if HAVE_OSS
      case settings::kAudioBackendOss: details += "oss ("; break;
#endif
#if HAVE_PULSEAUDIO
      case settings::kAudioBackendPulseaudio: details += "pulseaudio ("; break;
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

Glib::RefPtr<Application> Application::create()
{
  return Glib::RefPtr<Application>(new Application());
}

Application::Application() : Gtk::Application("org.gmetronome")
{}

Application::~Application()
{}

void Application::on_startup()
{
  //call the base class's implementation
  Gtk::Application::on_startup();

  auto desktop_info = Gio::DesktopAppInfo::create("gmetronome.desktop");
  if (desktop_info)
  {
    auto name = desktop_info->get_locale_string("Name");
    if (!name.empty())
      Glib::set_application_name(name);
    else
      Glib::set_application_name(PACKAGE_NAME);
  }
  else Glib::set_application_name(PACKAGE_NAME);

  // initialize application settings (GSettings)
  initSettings();
  // initialize application actions
  initActions();
  // initialize ticker
  initTicker();
  // initialize profile manager
  initProfiles();
  // initialize UI (i.e.MainWindow)
  initUI();
}

void Application::on_activate()
{
  main_window_->present();
}

void Application::initActions()
{
  const ActionHandlerMap kAppActionHandler =
    {
      {kActionQuit,            sigc::mem_fun(*this, &Application::onQuit)},

      {kActionVolume,          settings_prefs_},
      {kActionVolumeIncrease,  sigc::mem_fun(*this, &Application::onVolumeIncrease)},
      {kActionVolumeDecrease,  sigc::mem_fun(*this, &Application::onVolumeDecrease)},

      {kActionStart,           sigc::mem_fun(*this, &Application::onStart)},
      {kActionTempo,           sigc::mem_fun(*this, &Application::onTempo)},
      {kActionTempoIncrease,   sigc::mem_fun(*this, &Application::onTempoIncrease)},
      {kActionTempoDecrease,   sigc::mem_fun(*this, &Application::onTempoDecrease)},
      {kActionTempoTap,        sigc::mem_fun(*this, &Application::onTempoTap)},
      {kActionTrainerEnabled,  sigc::mem_fun(*this, &Application::onTrainerEnabled)},
      {kActionTrainerStart,    sigc::mem_fun(*this, &Application::onTrainerStart)},
      {kActionTrainerTarget,   sigc::mem_fun(*this, &Application::onTrainerTarget)},
      {kActionTrainerAccel,    sigc::mem_fun(*this, &Application::onTrainerAccel)},

      {kActionMeterEnabled,    sigc::mem_fun(*this, &Application::onMeterEnabled)},
      {kActionMeterSelect,     sigc::mem_fun(*this, &Application::onMeterSelect)},
      {kActionMeterSimple2,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple2)},
      {kActionMeterSimple3,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple3)},
      {kActionMeterSimple4,    sigc::mem_fun(*this, &Application::onMeterChanged_Simple4)},
      {kActionMeterCompound2,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound2)},
      {kActionMeterCompound3,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound3)},
      {kActionMeterCompound4,  sigc::mem_fun(*this, &Application::onMeterChanged_Compound4)},
      {kActionMeterCustom,     sigc::mem_fun(*this, &Application::onMeterChanged_Custom)},

      {kActionProfilesList,         sigc::mem_fun(*this, &Application::onProfilesList)},
      {kActionProfilesSelect,       sigc::mem_fun(*this, &Application::onProfilesSelect)},
      {kActionProfilesNew,          sigc::mem_fun(*this, &Application::onProfilesNew)},
      {kActionProfilesDelete,       sigc::mem_fun(*this, &Application::onProfilesDelete)},
      {kActionProfilesReset,        sigc::mem_fun(*this, &Application::onProfilesReset)},
      {kActionProfilesTitle,        sigc::mem_fun(*this, &Application::onProfilesTitle)},
      {kActionProfilesDescription,  sigc::mem_fun(*this, &Application::onProfilesDescription)},
      {kActionProfilesReorder,      sigc::mem_fun(*this, &Application::onProfilesReorder)},

      {kActionAudioDeviceList,      sigc::mem_fun(*this, &Application::onAudioDeviceList)}
    };

  install_actions(*this, kActionDescriptions, kAppActionHandler);
}

void Application::initSettings()
{
  settings_ = Gio::Settings::create(settings::kSchemaId);
  settings_prefs_ = settings_->get_child(settings::kSchemaIdPrefsBasename);
  settings_state_ = settings_->get_child(settings::kSchemaIdStateBasename);
  settings_shortcuts_ = settings_prefs_->get_child(settings::kSchemaIdShortcutsBasename);

  settings_prefs_connection_ = settings_prefs_->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsPrefsChanged));

  settings_state_connection_ = settings_state_->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsStateChanged));

  settings_shortcuts_connection_ = settings_shortcuts_->signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onSettingsShortcutsChanged));
}

void Application::initProfiles()
{
  profiles_manager_.signal_changed()
    .connect(sigc::mem_fun(*this, &Application::onProfilesManagerChanged));

  profiles_manager_.setIOModule(std::make_unique<ProfilesIOLocalXml>());

  Glib::ustring restore_profile_id = "";

  if (settings_prefs_->get_boolean(settings::kKeyPrefsRestoreProfile))
    restore_profile_id = settings_state_->get_string(settings::kKeyStateProfilesSelect);

  Glib::Variant<Glib::ustring> state
    = Glib::Variant<Glib::ustring>::create(restore_profile_id);

  activate_action(kActionProfilesSelect, state);

  if ( restore_profile_id.empty() )
    loadDefaultProfile();
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
    Glib::ustring accel = settings_shortcuts_->get_string(key);

    auto action = kActionDescriptions.find(shortcut_action.action_name);
    if (action != kActionDescriptions.end())
    {
      setAccelerator( action->second.scope,
                      shortcut_action.action_name,
                      shortcut_action.target_value,
                      accel );
    }
  };
}

void Application::initTicker()
{
  configureTickerSound();
  configureAudioBackend();
}

void Application::configureTickerSound()
{
  configureTickerSoundStrong();
  configureTickerSoundMid();
  configureTickerSoundWeak();
}

void Application::configureTickerSoundStrong()
{
  constexpr double maxvol2 = settings::kMaxVolume * settings::kMaxVolume;

  ticker_.setSoundStrong(settings_prefs_->get_double(settings::kKeyPrefsSoundStrongFrequency),
                         settings_prefs_->get_double(settings::kKeyPrefsSoundStrongVolume) *
                         settings_prefs_->get_double(settings::kKeyPrefsVolume) / maxvol2,
                         settings_prefs_->get_double(settings::kKeyPrefsSoundStrongBalance));
}

void Application::configureTickerSoundMid()
{
  constexpr double maxvol2 = settings::kMaxVolume * settings::kMaxVolume;

  ticker_.setSoundMid(settings_prefs_->get_double(settings::kKeyPrefsSoundMidFrequency),
                      settings_prefs_->get_double(settings::kKeyPrefsSoundMidVolume) *
                      settings_prefs_->get_double(settings::kKeyPrefsVolume) / maxvol2,
                      settings_prefs_->get_double(settings::kKeyPrefsSoundMidBalance));
}

void Application::configureTickerSoundWeak()
{
  constexpr double maxvol2 = settings::kMaxVolume * settings::kMaxVolume;

  ticker_.setSoundWeak(settings_prefs_->get_double(settings::kKeyPrefsSoundWeakFrequency),
                       settings_prefs_->get_double(settings::kKeyPrefsSoundWeakVolume) *
                       settings_prefs_->get_double(settings::kKeyPrefsVolume) / maxvol2,
                       settings_prefs_->get_double(settings::kKeyPrefsSoundWeakBalance));
}

void Application::configureAudioBackend()
{
  bool error = false;
  Message error_message;

  try {
    settings::AudioBackend backend = (settings::AudioBackend)
      settings_prefs_->get_enum(settings::kKeyPrefsAudioBackend);

    auto new_backend = audio::createBackend( backend );

    if (new_backend)
    {
      auto audio_devices = new_backend->devices();

      std::vector<Glib::ustring> dev_list;
      dev_list.reserve(audio_devices.size());

      for (auto& dev : audio_devices)
        dev_list.push_back(dev.name);

      Glib::Variant<std::vector<Glib::ustring>> dev_list_state
        = Glib::Variant<std::vector<Glib::ustring>>::create(dev_list);

      lookup_simple_action(kActionAudioDeviceList)->set_state(dev_list_state);

      auto device_config = audio::kDefaultConfig;
      device_config.name = currentAudioDevice();

      new_backend->configure(device_config);
    }
    ticker_.setBackend( std::move(new_backend) );
  }
  catch(const audio::BackendError& e)
  {
    error = true;
    error_message = kAudioBackendErrorMessage;
    error_message.details = getErrorDetails(std::current_exception());
  }
  catch(const std::exception& e)
  {
    error = true;
    error_message = kAudioBackendErrorMessage;
    error_message.details = getErrorDetails(std::current_exception());
  }

  if (error)
  {
    ticker_.setBackend( nullptr ); // use dummy backend
    signal_message_.emit(error_message);
  }
}

void Application::configureAudioDevice()
{
  auto cfg = audio::kDefaultConfig;
  cfg.name = currentAudioDevice();
  ticker_.configureAudioDevice(cfg);
}

Glib::RefPtr<Gio::SimpleAction> Application::lookup_simple_action(const Glib::ustring& name)
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

  bool start_state;
  get_action_state(kActionStart, start_state);

  if (start_state)
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

void Application::onTrainerEnabled(const Glib::VariantBase& value)
{
  Glib::Variant<bool> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);

  if (new_state.get())
  {
    double trainer_target_tempo;
    get_action_state(kActionTrainerTarget, trainer_target_tempo);

    double trainer_accel;
    get_action_state(kActionTrainerAccel, trainer_accel);

    ticker_.setTargetTempo(trainer_target_tempo);
    ticker_.setAccel(trainer_accel);
  }
  else
  {
    double tempo;
    get_action_state(kActionTempo, tempo);

    ticker_.setTargetTempo(tempo);
    ticker_.setAccel(0.0);
    ticker_.setTempo(tempo);
  }

  lookup_simple_action(kActionTrainerEnabled)->set_state(new_state);
}

void Application::onMeterEnabled(const Glib::VariantBase& value)
{
  Glib::Variant<bool> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);

  if (new_state.get())
  {
    Glib::ustring current_meter_slot;
    get_action_state(kActionMeterSelect, current_meter_slot);

    Meter meter;
    get_action_state(current_meter_slot, meter);

    ticker_.setMeter(std::move(meter));
  }
  else
  {
    ticker_.setMeter( kMeter1 );
  }

  lookup_simple_action(kActionMeterEnabled)->set_state(new_state);
}

void Application::onMeterSelect(const Glib::VariantBase& value)
{
  auto in_meter_slot =
    Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

  Glib::ustring current_meter_slot;
  get_action_state(kActionMeterSelect, current_meter_slot);

  if (in_meter_slot != current_meter_slot)
  {
    if (auto [new_meter_slot, valid] = validateMeterSlot(std::move(in_meter_slot));
        valid)
    {
      bool meter_enabled;
      get_action_state(kActionMeterEnabled, meter_enabled);

      if (meter_enabled)
      {
        Meter meter;
        get_action_state(new_meter_slot, meter);

        ticker_.setMeter(std::move(meter));
      }

      Glib::Variant<Glib::ustring> out_state =
        Glib::Variant<Glib::ustring>::create(new_meter_slot);

      lookup_simple_action(kActionMeterSelect)->set_state(out_state);
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
  Meter old_meter;
  get_action_state(action_name, old_meter);

  Glib::Variant<Meter> in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Meter>>(value);

  Meter new_meter = in_state.get();

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

void Application::onMeterChanged_SetState(const Glib::ustring& action_name,
                                          Meter&& in_meter)
{
  auto action = lookup_simple_action(action_name);
  if (action)
  {
    bool meter_enabled;
    get_action_state(kActionMeterEnabled, meter_enabled);

    Glib::ustring current_meter_slot;
    get_action_state(kActionMeterSelect, current_meter_slot);

    auto [meter, valid] = validateMeter(in_meter);

    Glib::Variant<Meter> out_state = Glib::Variant<Meter>::create(meter);

    if (meter_enabled == true && current_meter_slot == action_name)
    {
      ticker_.setMeter(std::move(meter));
    }

    action->set_state(out_state);
  }
}

void Application::onVolumeIncrease(const Glib::VariantBase& value)
{
  double delta_volume
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double current_volume = settings_prefs_->get_double(settings::kKeyPrefsVolume);

  auto [new_volume, valid] = validateVolume(current_volume + delta_volume);

  settings_prefs_->set_double(settings::kKeyPrefsVolume, new_volume);
}

void Application::onVolumeDecrease(const Glib::VariantBase& value)
{
  double delta_volume
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double current_volume = settings_prefs_->get_double(settings::kKeyPrefsVolume);

  auto [new_volume, valid] = validateVolume(current_volume - delta_volume);

  settings_prefs_->set_double(settings::kKeyPrefsVolume, new_volume);
}

void Application::onTempo(const Glib::VariantBase& value)
{
  double in_tempo =
    Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  auto [tempo, valid] = validateTempo(in_tempo);

  ticker_.setTempo(tempo);

  auto new_state = Glib::Variant<double>::create(tempo);

  lookup_simple_action(kActionTempo)->set_state(new_state);
}

void Application::onTempoIncrease(const Glib::VariantBase& value)
{
  double delta_tempo
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double tempo_state;
  get_action_state(kActionTempo, tempo_state);

  tempo_state += delta_tempo;

  Glib::Variant<double> new_tempo_state
    = Glib::Variant<double>::create(tempo_state);

  activate_action(kActionTempo, new_tempo_state);
}

void Application::onTempoDecrease(const Glib::VariantBase& value)
{
  double delta_tempo
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();

  double tempo_state;
  get_action_state(kActionTempo, tempo_state);

  tempo_state -= delta_tempo;

  Glib::Variant<double> new_tempo_state
    = Glib::Variant<double>::create(tempo_state);

  activate_action(kActionTempo, new_tempo_state);
}

void Application::onTempoTap(const Glib::VariantBase& value)
{
  using std::literals::chrono_literals::operator""min;

  static auto now = std::chrono::steady_clock::now();
  static auto last_timepoint = now;

  now = std::chrono::steady_clock::now();

  auto duration = now - last_timepoint;

  if ( duration.count() > 0 )
  {
    double bpm = 1min / duration;

    if ( bpm >= Profile::kMinTempo && bpm <= Profile::kMaxTempo )
    {
      Glib::Variant<double> new_tempo_state = Glib::Variant<double>::create( bpm );
      activate_action(kActionTempo, new_tempo_state);
    }
  }
  last_timepoint = now;
}

void Application::onTrainerStart(const Glib::VariantBase& value)
{
  double in_tempo = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [tempo, valid] = validateTrainerStart(in_tempo);

  auto new_state = Glib::Variant<double>::create(tempo);
  lookup_simple_action(kActionTrainerStart)->set_state(new_state);
}

void Application::onTrainerTarget(const Glib::VariantBase& value)
{
  double in_tempo = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [tempo, valid] = validateTrainerTarget(in_tempo);

  bool trainer_enabled;
  get_action_state(kActionTrainerEnabled, trainer_enabled);

  if (trainer_enabled)
    ticker_.setTargetTempo(tempo);

  auto new_state = Glib::Variant<double>::create(tempo);
  lookup_simple_action(kActionTrainerTarget)->set_state(new_state);
}

void Application::onTrainerAccel(const Glib::VariantBase& value)
{
  double in_accel = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  auto [accel, valid] = validateTrainerAccel(in_accel);

  bool trainer_enabled;
  get_action_state(kActionTrainerEnabled, trainer_enabled);

  if (trainer_enabled)
    ticker_.setAccel(accel);

  auto new_state = Glib::Variant<double>::create(accel);
  lookup_simple_action(kActionTrainerAccel)->set_state(new_state);
}

void Application::onProfilesManagerChanged()
{
  auto in_list = profiles_manager_.profileList();

  ProfilesList out_list;

  std::transform(in_list.begin(), in_list.end(), std::back_inserter(out_list),
                 [] (const auto& primer) -> ProfilesListEntry {
                   return {primer.id, primer.header.title, primer.header.description};
                 });

  Glib::Variant<ProfilesList> out_list_state
    = Glib::Variant<ProfilesList>::create(out_list);

  lookup_simple_action(kActionProfilesList)->set_state(out_list_state);

  // update selection
  Glib::ustring selected_id;
  get_action_state(kActionProfilesSelect, selected_id);

  if ( !selected_id.empty() )
  {
    auto it = std::find_if(out_list.begin(), out_list.end(),
                           [&selected_id] (const auto& p) -> bool {
                             auto& id = std::get<kProfilesListEntryIdentifier>(p);
                             return id == selected_id;
                           });

    if (it != out_list.end())
    {
      Glib::Variant<Glib::ustring> title_state
        = Glib::Variant<Glib::ustring>::create(std::get<kProfilesListEntryTitle>(*it));

      Glib::Variant<Glib::ustring> descr_state
        = Glib::Variant<Glib::ustring>::create(std::get<kProfilesListEntryDescription>(*it));

      lookup_simple_action(kActionProfilesTitle)->set_state(title_state);
      lookup_simple_action(kActionProfilesDescription)->set_state(descr_state);
    }
    else
    {
      Glib::Variant<Glib::ustring> empty_state
        = Glib::Variant<Glib::ustring>::create({""});

      activate_action(kActionProfilesSelect, empty_state);
    }
  }
}

void Application::onProfilesList(const Glib::VariantBase& value)
{
  // The "profiles-list" action state is modified in response to "signal_changed"
  // of the profiles manager. (see onProfilesManagerChanged() signal handler)
  // It gives clients access to an up-to-date list of all available profiles
  // but can not be modified by clients via activate_action() or change_action_state().
}

void Application::onProfilesSelect(const Glib::VariantBase& value)
{
  saveSelectedProfile();

  Glib::Variant<Glib::ustring> in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

  ProfilesList plist;
  get_action_state(kActionProfilesList, plist);

  if ( in_state.get().empty() )
  {
    lookup_simple_action(kActionProfilesDelete)->set_enabled(false);
    lookup_simple_action(kActionProfilesTitle)->set_enabled(false);
    lookup_simple_action(kActionProfilesDescription)->set_enabled(false);

    Glib::Variant<Glib::ustring> empty_state
      = Glib::Variant<Glib::ustring>::create({""});

    lookup_simple_action(kActionProfilesSelect)->set_state(empty_state);
    lookup_simple_action(kActionProfilesTitle)->set_state(empty_state);
    lookup_simple_action(kActionProfilesDescription)->set_state(empty_state);
  }
  else
  {
    auto it = std::find_if(plist.begin(), plist.end(),
                           [&in_state] (auto& e) {
                             return std::get<kProfilesListEntryIdentifier>(e) == in_state.get();
                           });
    if (it != plist.end())
    {
      lookup_simple_action(kActionProfilesSelect)->set_state(in_state);

      Glib::ustring& title = std::get<kProfilesListEntryTitle>(*it);
      Glib::ustring& description = std::get<kProfilesListEntryDescription>(*it);

      lookup_simple_action(kActionProfilesTitle)
        ->set_state(Glib::Variant<Glib::ustring>::create( title ));

      lookup_simple_action(kActionProfilesDescription)
        ->set_state(Glib::Variant<Glib::ustring>::create( description ));

      lookup_simple_action(kActionProfilesDelete)->set_enabled(true);
      lookup_simple_action(kActionProfilesTitle)->set_enabled(true);
      lookup_simple_action(kActionProfilesDescription)->set_enabled(true);
    }
  }

  Glib::ustring selected_id;
  get_action_state(kActionProfilesSelect, selected_id);

  settings_state_connection_.block();
  settings_state_->set_string(settings::kKeyStateProfilesSelect, selected_id);
  settings_state_connection_.unblock();

  loadSelectedProfile();
}

void Application::convertActionToProfile(Profile::Content& content)
{
  get_action_state(kActionTempo, content.tempo);
  get_action_state(kActionMeterEnabled, content.meter_enabled);

  Glib::ustring tmp_string;
  get_action_state(kActionMeterSelect,  tmp_string);
  content.meter_select = tmp_string.raw();

  get_action_state(kActionMeterSimple2, content.meter_simple_2);
  get_action_state(kActionMeterSimple3, content.meter_simple_3);
  get_action_state(kActionMeterSimple4, content.meter_simple_4);
  get_action_state(kActionMeterCompound2, content.meter_compound_2);
  get_action_state(kActionMeterCompound3, content.meter_compound_3);
  get_action_state(kActionMeterCompound4, content.meter_compound_4);
  get_action_state(kActionMeterCustom, content.meter_custom);
  get_action_state(kActionTrainerEnabled, content.trainer_enabled);
  get_action_state(kActionTrainerStart, content.trainer_start);
  get_action_state(kActionTrainerTarget, content.trainer_target);
  get_action_state(kActionTrainerAccel, content.trainer_accel);
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
  activate_action(kActionTrainerStart,
                  Glib::Variant<double>::create(content.trainer_start) );
  activate_action(kActionTrainerTarget,
                  Glib::Variant<double>::create(content.trainer_target) );
  activate_action(kActionTrainerAccel,
                  Glib::Variant<double>::create(content.trainer_accel) );
}

void Application::onProfilesNew(const Glib::VariantBase& value)
{
  Glib::Variant<Glib::ustring> in_title
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

  auto [title,valid] = validateProfileTitle(in_title.get());

  Profile::Header header = {title, ""};
  Profile::Content content;

  convertActionToProfile(content);

  Profile::Primer primer = profiles_manager_.newProfile(header, content);

  auto id = Glib::Variant<Glib::ustring>::create(primer.id);

  activate_action(kActionProfilesSelect, id);
}

void Application::loadSelectedProfile()
{
  Glib::ustring id;
  get_action_state(kActionProfilesSelect, id);

  if (!id.empty())
  {
    Profile::Content content = profiles_manager_.getProfileContent(id);

    convertProfileToAction(content);
  }
}

void Application::loadDefaultProfile()
{
  convertProfileToAction(kDefaultProfile.content);
}

void Application::saveSelectedProfile()
{
  Glib::ustring id;
  get_action_state(kActionProfilesSelect, id);

  if (!id.empty())
  {
    Profile::Content content;

    convertActionToProfile(content);

    try {
      profiles_manager_.setProfileContent(id, content);
    }
    catch (...) {
      // the profile does not exist (anymore);
      // e.g. this happens after a delete operation ('profile-delete')
    }
  }
}

void Application::onProfilesDelete(const Glib::VariantBase& value)
{
  Glib::ustring id;
  get_action_state(kActionProfilesSelect, id);

  if (!id.empty())
  {
    profiles_manager_.deleteProfile(id);
  }
}

void Application::onProfilesReset(const Glib::VariantBase& value)
{
  loadDefaultProfile();
}

void Application::onProfilesTitle(const Glib::VariantBase& value)
{
  Glib::ustring id;
  get_action_state(kActionProfilesSelect, id);

  if (!id.empty())
  {
    Glib::Variant<Glib::ustring> in_value
      = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    auto [title, valid] = validateProfileTitle(in_value.get());

    Glib::Variant<Glib::ustring> out_value
      = Glib::Variant<Glib::ustring>::create(title);

    Profile::Header header = profiles_manager_.getProfileHeader(id);
    header.title = out_value.get();

    profiles_manager_.setProfileHeader(id, header);

    lookup_simple_action(kActionProfilesTitle)->set_state(out_value);
  }
}

void Application::onProfilesDescription(const Glib::VariantBase& value)
{
  Glib::ustring id;
  get_action_state(kActionProfilesSelect, id);

  if (!id.empty())
  {
    Glib::Variant<Glib::ustring> in_value
      = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    Glib::Variant<Glib::ustring> out_value;

    // validate in_value
#if GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 62
    out_value = Glib::Variant<Glib::ustring>
      ::create(in_value.get().make_valid().substr(0,Profile::kDescriptionMaxLength));
#else
    if (in_value.get().validate())
      out_value = Glib::Variant<Glib::ustring>
        ::create(in_value.get().substr(0,Profile::kDescriptionMaxLength));
    else
      out_value = Glib::Variant<Glib::ustring>
        ::create(Profile::Header().description); // default description
#endif

    Profile::Header header = profiles_manager_.getProfileHeader(id);
    header.description = out_value.get();

    profiles_manager_.setProfileHeader(id, header);

    lookup_simple_action(kActionProfilesDescription)->set_state(out_value);
  }
}

void Application::onProfilesReorder(const Glib::VariantBase& value)
{
  Glib::Variant<ProfilesIdentifierList> in_list
    = Glib::VariantBase::cast_dynamic<Glib::Variant<ProfilesIdentifierList>>(value);

  std::vector<Profile::Identifier> out_list;

  // convert ProfilesIdentifierList to std::vector<Profile::Identifier>
  auto iter = in_list.get_iter();
  out_list.reserve(in_list.get_n_children());

  Glib::Variant<ProfilesIdentifierList::value_type> id;
  while (iter.next_value(id))
      out_list.push_back(id.get());

  profiles_manager_.reorderProfiles(out_list);
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
      bool trainer_enabled;
      get_action_state(kActionTrainerEnabled, trainer_enabled);

      if (trainer_enabled)
      {
        double trainer_start_tempo;
        get_action_state(kActionTrainerStart, trainer_start_tempo);

        ticker_.setTempo(trainer_start_tempo);
      }
      ticker_.start();
      startTimer();
    }
    else
    {
      stopTimer();
      ticker_.stop();
    }
  }
  catch(const audio::BackendError& e)
  {
    error = true;
    error_message = kAudioBackendErrorMessage;
    error_message.details = getErrorDetails(std::current_exception());
  }
  catch(const std::exception& e)
  {
    error = true;
    error_message = kGenericErrorMessage;
    error_message.details = getErrorDetails(std::current_exception());
  }

  if (error)
  {
    ticker_.reset();
    new_state = Glib::Variant<bool>::create(false);
    signal_message_.emit(error_message);
  }

  lookup_simple_action(kActionStart)->set_state(new_state);
}

void Application::onAudioDeviceList(const Glib::VariantBase& value)
{
  // The state of kActionAudioDeviceList provides a list of audio devices
  // as given by the current audio backend. It is not to be changed in
  // response of an "activation" or a "change_state" request of the client.
  // Ergo: nothing to do here
}

Glib::ustring Application::currentAudioDeviceKey()
{
  settings::AudioBackend backend = (settings::AudioBackend)
    settings_prefs_->get_enum(settings::kKeyPrefsAudioBackend);

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
    return settings_prefs_->get_string(key);
  else
    return "";
}

void Application::onSettingsPrefsChanged(const Glib::ustring& key)
{
  if (key == settings::kKeyPrefsVolume)
  {
    configureTickerSound();
  }
  else if (key == settings::kKeyPrefsSoundStrongFrequency
           || key == settings::kKeyPrefsSoundStrongVolume
           || key == settings::kKeyPrefsSoundStrongBalance)
  {
    configureTickerSoundStrong();
  }
  else if (key == settings::kKeyPrefsSoundMidFrequency
           || key == settings::kKeyPrefsSoundMidVolume
           || key == settings::kKeyPrefsSoundMidBalance)
  {
    configureTickerSoundMid();
  }
  else if (key == settings::kKeyPrefsSoundWeakFrequency
           || key == settings::kKeyPrefsSoundWeakVolume
           || key == settings::kKeyPrefsSoundWeakBalance)
  {
    configureTickerSoundWeak();
  }
  else if (key == settings::kKeyPrefsAudioBackend)
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
  if (key == settings::kKeyStateProfilesSelect)
  {
    Glib::ustring profile_id = settings_state_->get_string(settings::kKeyStateProfilesSelect);

    Glib::Variant<Glib::ustring> state
      = Glib::Variant<Glib::ustring>::create(profile_id);

    activate_action(kActionProfilesSelect, state);
  }
}

void Application::onSettingsShortcutsChanged(const Glib::ustring& key)
{
  auto shortcut_action = kDefaultShortcutActionMap.find(key);
  if (shortcut_action != kDefaultShortcutActionMap.end())
  {
    Glib::ustring accel = settings_shortcuts_->get_string(key);

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

void Application::startTimer()
{
  timer_connection_ = Glib::signal_timeout()
    .connect(sigc::mem_fun(*this, &Application::onTimer), 70);
}

void Application::stopTimer()
{
  using std::literals::chrono_literals::operator""us;

  timer_connection_.disconnect();
  signal_ticker_statistics_.emit({ 0us, 0.0, 0.0, -1.0, -1, 0us, 0us });
}

bool Application::onTimer()
{
  using std::literals::chrono_literals::operator""us;

  audio::TickerState state = ticker_.state();
  if (state.test(audio::TickerStateFlag::kError))
  {
    // this will handle the error
    change_action_state(kActionStart, Glib::Variant<bool>::create(false));
    return false;
  }
  else
  {
    audio::Ticker::Statistics stats = ticker_.getStatistics();

    bool meter_enabled;
    get_action_state(kActionMeterEnabled, meter_enabled);

    if (!meter_enabled)
    {
      stats.next_accent = -1;
      stats.next_accent_delay = 0us;
    }

    signal_ticker_statistics_.emit(stats);
    return true;
  }
}

// helper
template<class T>
std::pair<T,bool> validateRange(T value, const ActionStateHintRange<T>& range)
{
  T ret = clampActionStateValue(value, range);
  return { ret, value == ret };
}

template<class T>
std::pair<T,bool> validateRange(T value, const T& min, const T& max)
{
  T ret = std::clamp(value, min, max);
  return { ret, value == ret };
}

std::pair<double,bool> Application::validateTempo(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTempo, range);
  return validateRange(value, range);
}

std::pair<double,bool> Application::validateTrainerStart(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerStart, range);
  return validateRange(value, range);
}

std::pair<double,bool> Application::validateTrainerTarget(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerTarget, range);
  return validateRange(value, range);
}

std::pair<double,bool> Application::validateTrainerAccel(double value)
{
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTrainerAccel, range);
  return validateRange(value, range);
}

std::pair<double,bool> Application::validateVolume(double value)
{
  return validateRange(value, settings::kMinVolume, settings::kMaxVolume);
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
  {
    Glib::ustring current_meter_slot;
    get_action_state(kActionMeterSelect, current_meter_slot);
    return {current_meter_slot, false};
  }
}

std::pair<Glib::ustring,bool> Application::validateProfileTitle(Glib::ustring str)
{
  Glib::ustring ret;
#if GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 62
    ret = str.make_valid().substr(0, Profile::kTitleMaxLength);
#else
    if (str.validate())
      ret = str.substr(0, Profile::kTitleMaxLength);
    else
      ret = "";
#endif
    return {ret, ret.raw() == str.raw()};
}
