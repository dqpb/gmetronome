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
#include <iostream>

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
  }
  
  // initialize application settings (GSettings)
  initSettings();
  // initialize application actions
  initActions();
  // initialize profile manager
  initProfiles();
  // initialize ticker
  initTicker();
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
      {kActionMeter1Simple,    sigc::mem_fun(*this, &Application::onMeterChanged_1_Simple)},
      {kActionMeter2Simple,    sigc::mem_fun(*this, &Application::onMeterChanged_2_Simple)},
      {kActionMeter3Simple,    sigc::mem_fun(*this, &Application::onMeterChanged_3_Simple)},
      {kActionMeter4Simple,    sigc::mem_fun(*this, &Application::onMeterChanged_4_Simple)},
      {kActionMeter1Compound,  sigc::mem_fun(*this, &Application::onMeterChanged_1_Compound)},
      {kActionMeter2Compound,  sigc::mem_fun(*this, &Application::onMeterChanged_2_Compound)},
      {kActionMeter3Compound,  sigc::mem_fun(*this, &Application::onMeterChanged_3_Compound)},
      {kActionMeter4Compound,  sigc::mem_fun(*this, &Application::onMeterChanged_4_Compound)},
      {kActionMeterCustom,     sigc::mem_fun(*this, &Application::onMeterChanged_Custom)},
      
      {kActionProfilesList,         sigc::mem_fun(*this, &Application::onProfilesList)},
      {kActionProfilesSelect,       sigc::mem_fun(*this, &Application::onProfilesSelect)},
      {kActionProfilesNew,          sigc::mem_fun(*this, &Application::onProfilesNew)},
      {kActionProfilesDelete,       sigc::mem_fun(*this, &Application::onProfilesDelete)},
      {kActionProfilesReset,        sigc::mem_fun(*this, &Application::onProfilesReset)},
      {kActionProfilesTitle,        sigc::mem_fun(*this, &Application::onProfilesTitle)},
      {kActionProfilesDescription,  sigc::mem_fun(*this, &Application::onProfilesDescription)}
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

  if (settings_prefs_->get_boolean(settings::kKeyPrefsRestoreProfile))
  {
    Glib::ustring profile_id = settings_state_->get_string(settings::kKeyStateProfilesSelect);
    
    Glib::Variant<Glib::ustring> state
      = Glib::Variant<Glib::ustring>::create(profile_id);
    
    activate_action(kActionProfilesSelect, state);
  }
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
}

void Application::configureTickerSound()
{
  configureTickerSoundStrong();
  configureTickerSoundMid();
  configureTickerSoundWeak();
}

void Application::configureTickerSoundStrong()
{
  ticker_.setSoundStrong(settings_prefs_->get_double(settings::kKeyPrefsSoundStrongFrequency),
                         settings_prefs_->get_double(settings::kKeyPrefsSoundStrongVolume) *
                         settings_prefs_->get_double(settings::kKeyPrefsVolume) / 100. / 100.); 
}

void Application::configureTickerSoundMid()
{
  ticker_.setSoundMid(settings_prefs_->get_double(settings::kKeyPrefsSoundMidFrequency),
                      settings_prefs_->get_double(settings::kKeyPrefsSoundMidVolume) *
                      settings_prefs_->get_double(settings::kKeyPrefsVolume) / 100. / 100.); 
}

void Application::configureTickerSoundWeak()
{
  ticker_.setSoundWeak(settings_prefs_->get_double(settings::kKeyPrefsSoundWeakFrequency),
                       settings_prefs_->get_double(settings::kKeyPrefsSoundWeakVolume) *
                       settings_prefs_->get_double(settings::kKeyPrefsVolume) / 100. / 100.); 
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
  {
    set_accel_for_action(detailed_action_name, accel);
  }
  else
  {
    unset_accels_for_action(detailed_action_name);
  }
}

void Application::onHideWindow(Gtk::Window* window)
{
  saveSelectedProfile();
  
  bool start_state;
  get_action_state(kActionStart, start_state);
  
  if (start_state)
    activate_action(kActionStart);
  
  delete window;
}

void Application::onQuit(const Glib::VariantBase& parameter)
{  
  // Gio::Application::quit() will make Gio::Application::run() return,
  // but it's a crude way of ending the program. The window is not removed
  // from the application. Neither the window's nor the application's
  // destructors will be called, because there will be remaining reference
  // counts in both of them. If we want the destructors to be called, we
  // must remove the window from the application. One way of doing this
  // is to hide the window. See comment in create_appwindow().
  auto windows = get_windows();
  
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
    ticker_.setMeter( kMeter_1 );
  }
  
  lookup_simple_action(kActionMeterEnabled)->set_state(new_state);
}

void Application::onMeterSelect(const Glib::VariantBase& value)
{
  auto in_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);
  
  Glib::ustring new_meter_slot = in_state.get();
  
  Glib::ustring current_meter_slot;
  get_action_state(kActionMeterSelect, current_meter_slot);
  
  if (new_meter_slot != current_meter_slot)
  {
    // check in_state validity
    if (new_meter_slot == kActionMeter1Simple
        || new_meter_slot == kActionMeter2Simple
        || new_meter_slot == kActionMeter3Simple
        || new_meter_slot == kActionMeter4Simple
        || new_meter_slot == kActionMeter1Compound
        || new_meter_slot == kActionMeter2Compound
        || new_meter_slot == kActionMeter3Compound
        || new_meter_slot == kActionMeter4Compound
        || new_meter_slot == kActionMeterCustom )
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

void Application::onMeterChanged_1_Simple(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter1Simple, value); }

void Application::onMeterChanged_2_Simple(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter2Simple, value); }

void Application::onMeterChanged_3_Simple(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter3Simple, value); }

void Application::onMeterChanged_4_Simple(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter4Simple, value); }

void Application::onMeterChanged_1_Compound(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter1Compound, value); }

void Application::onMeterChanged_2_Compound(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter2Compound, value); }

void Application::onMeterChanged_3_Compound(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter3Compound, value); }

void Application::onMeterChanged_4_Compound(const Glib::VariantBase& value)
{ onMeterChanged_Default(kActionMeter4Compound, value); }

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
                                          Meter&& meter)
{
  auto action = lookup_simple_action(action_name);
  if (action)
  {
    bool meter_enabled;
    get_action_state(kActionMeterEnabled, meter_enabled);
    
    Glib::ustring current_meter_slot;
    get_action_state(kActionMeterSelect, current_meter_slot);

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
  double new_volume = current_volume + delta_volume;
  
  new_volume = std::clamp(new_volume, kMinimumVolume, kMaximumVolume);
  
  settings_prefs_->set_double(settings::kKeyPrefsVolume, new_volume);
}

void Application::onVolumeDecrease(const Glib::VariantBase& value)
{
  double delta_volume
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value).get();
  
  double current_volume = settings_prefs_->get_double(settings::kKeyPrefsVolume);
  double new_volume = current_volume - delta_volume;
  
  new_volume = std::clamp(new_volume, kMinimumVolume, kMaximumVolume);
  
  settings_prefs_->set_double(settings::kKeyPrefsVolume, new_volume);
}

void Application::onTempo(const Glib::VariantBase& in_val)
{
  double value =
    Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(in_val).get();
  
  ActionStateHintRange<double> range;
  get_action_state_hint(kActionTempo, range);

  value = clampActionStateValue(value, range);
  
  ticker_.setTempo(value);

  auto new_state = Glib::Variant<double>::create(value);
  
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
    
    if ( bpm >= kMinimumTempo && bpm <= kMaximumTempo )
    {
      Glib::Variant<double> new_tempo_state = Glib::Variant<double>::create( bpm );
      activate_action(kActionTempo, new_tempo_state);
    }
  }
  
  last_timepoint = now;
}

void Application::onTrainerStart(const Glib::VariantBase& value)
{
  Glib::Variant<double> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value);

  lookup_simple_action(kActionTrainerStart)->set_state(new_state);
}

void Application::onTrainerTarget(const Glib::VariantBase& value)
{
  Glib::Variant<double> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value);

  bool trainer_enabled;
  get_action_state(kActionTrainerEnabled, trainer_enabled);
  
  if (trainer_enabled)
    ticker_.setTargetTempo(new_state.get());

  lookup_simple_action(kActionTrainerTarget)->set_state(new_state);
}

void Application::onTrainerAccel(const Glib::VariantBase& value)
{
  Glib::Variant<double> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(value);

  bool trainer_enabled;
  get_action_state(kActionTrainerEnabled, trainer_enabled);
  
  if (trainer_enabled)
    ticker_.setAccel(new_state.get());

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
  // The "profiles-list" action state is modified in response to the "changed"
  // signal (signal_changed) of the profiles manager (i.e. the underlying storage)
  // and therefore gives clients access to an up-to-date list of all available
  // profiles. Currently it cannot be modified directly.
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
  
  get_action_state(kActionMeter1Simple, content.meter_1_simple);
  get_action_state(kActionMeter2Simple, content.meter_2_simple);
  get_action_state(kActionMeter3Simple, content.meter_3_simple);
  get_action_state(kActionMeter4Simple, content.meter_4_simple);
  get_action_state(kActionMeter1Compound, content.meter_1_compound);
  get_action_state(kActionMeter2Compound, content.meter_2_compound);
  get_action_state(kActionMeter3Compound, content.meter_3_compound);
  get_action_state(kActionMeter4Compound, content.meter_4_compound);
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
  activate_action(kActionMeter1Simple,
                  Glib::Variant<Meter>::create(content.meter_1_simple) );
  activate_action(kActionMeter2Simple,
                  Glib::Variant<Meter>::create(content.meter_2_simple) );
  activate_action(kActionMeter3Simple,
                  Glib::Variant<Meter>::create(content.meter_3_simple) );
  activate_action(kActionMeter4Simple,
                  Glib::Variant<Meter>::create(content.meter_4_simple) );
  activate_action(kActionMeter1Compound,
                  Glib::Variant<Meter>::create(content.meter_1_compound) );
  activate_action(kActionMeter2Compound,
                  Glib::Variant<Meter>::create(content.meter_2_compound) );
  activate_action(kActionMeter3Compound,
                  Glib::Variant<Meter>::create(content.meter_3_compound) );
  activate_action(kActionMeter4Compound,
                  Glib::Variant<Meter>::create(content.meter_4_compound) );
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
  Profile::Content content;
  
  convertActionToProfile(content);
  
  Profile::Primer primer = profiles_manager_.newProfile({}, content);

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
    
    Glib::Variant<Glib::ustring> out_value;
    
#if GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 62
    out_value = Glib::Variant<Glib::ustring>
      ::create(in_value.get().make_valid().substr(0,kProfileTitleMaxLength));
#else
    if (in_value.get().validate()) 
      out_value = Glib::Variant<Glib::ustring>
        ::create(in_value.get().substr(0,kProfileTitleMaxLength));
    else
      out_value = Glib::Variant<Glib::ustring>
        ::create(Profile::Header().title); // default title
#endif
    
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
      ::create(in_value.get().make_valid().substr(0,kProfileDescriptionMaxLength));
#else
    if (in_value.get().validate()) 
      out_value = Glib::Variant<Glib::ustring>
        ::create(in_value.get().substr(0,kProfileDescriptionMaxLength));
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

void Application::onStart(const Glib::VariantBase& value)
{
  Glib::Variant<bool> new_state
    = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);

  if (new_state.get() && ticker_.state() == audio::TickerState::kReady)
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

  lookup_simple_action(kActionStart)->set_state(new_state);
}

void Application::onSettingsPrefsChanged(const Glib::ustring& key)
{
  if (key == settings::kKeyPrefsVolume)
  {
    configureTickerSound();
  }
  else if (key == settings::kKeyPrefsSoundStrongFrequency || key == settings::kKeyPrefsSoundStrongVolume)
  {
    configureTickerSoundStrong();
  }
  else if (key == settings::kKeyPrefsSoundMidFrequency || key == settings::kKeyPrefsSoundMidVolume)
  {
    configureTickerSoundMid();
  }
  else if (key == settings::kKeyPrefsSoundWeakFrequency || key == settings::kKeyPrefsSoundWeakVolume)
  {
    configureTickerSoundWeak();
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
  timer_connection_.disconnect();
  signal_ticker_statistics_.emit({0,0,-1,0,0});
}

bool Application::onTimer()
{
  audio::Statistics stats = ticker_.getStatistics();

  bool meter_enabled;
  get_action_state(kActionMeterEnabled, meter_enabled);
  
  if (!meter_enabled)
  {
    stats.next_accent = -1;
    stats.next_accent_time = 0;
  }  
  
  signal_ticker_statistics_.emit(stats);
  
  return true;
}
