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

#ifndef GMetronome_Action_h
#define GMetronome_Action_h

#include "Profile.h"
#include "Meter.h"

#include <giomm.h>
#include <tuple>
#include <vector>
#include <map>
#include <variant>

// Application actions
inline const Glib::ustring  kActionQuit                 {"quit"};
inline const Glib::ustring  kActionStart                {"start"};
inline const Glib::ustring  kActionVolume               {"volume"};
inline const Glib::ustring  kActionVolumeIncrease       {"volume-increase"};
inline const Glib::ustring  kActionVolumeDecrease       {"volume-decrease"};
inline const Glib::ustring  kActionTempo                {"tempo"};
inline const Glib::ustring  kActionTempoIncrease        {"tempo-increase"};
inline const Glib::ustring  kActionTempoDecrease        {"tempo-decrease"};
inline const Glib::ustring  kActionTempoTap             {"tempo-tap"};
inline const Glib::ustring  kActionTrainerEnabled       {"trainer-enabled"};
inline const Glib::ustring  kActionTrainerStart         {"trainer-start"};
inline const Glib::ustring  kActionTrainerTarget        {"trainer-target"};
inline const Glib::ustring  kActionTrainerAccel         {"trainer-accel"};
inline const Glib::ustring  kActionMeterEnabled         {"meter-enabled"};
inline const Glib::ustring  kActionMeterSelect          {"meter-select"};
inline const Glib::ustring  kActionMeter1Simple         {"meter-1-simple"};
inline const Glib::ustring  kActionMeter2Simple         {"meter-2-simple"};
inline const Glib::ustring  kActionMeter3Simple         {"meter-3-simple"};
inline const Glib::ustring  kActionMeter4Simple         {"meter-4-simple"};
inline const Glib::ustring  kActionMeter1Compound       {"meter-1-compound"};
inline const Glib::ustring  kActionMeter2Compound       {"meter-2-compound"};
inline const Glib::ustring  kActionMeter3Compound       {"meter-3-compound"};
inline const Glib::ustring  kActionMeter4Compound       {"meter-4-compound"};
inline const Glib::ustring  kActionMeterCustom          {"meter-custom"};
inline const Glib::ustring  kActionProfilesList         {"profiles-list"};
inline const Glib::ustring  kActionProfilesSelect       {"profiles-select"};
inline const Glib::ustring  kActionProfilesNew          {"profiles-new"};
inline const Glib::ustring  kActionProfilesDelete       {"profiles-delete"};
inline const Glib::ustring  kActionProfilesReset        {"profiles-reset"};
inline const Glib::ustring  kActionProfilesTitle        {"profiles-title"};
inline const Glib::ustring  kActionProfilesDescription  {"profiles-description"};
inline const Glib::ustring  kActionProfilesReorder      {"profiles-reorder"};
inline const Glib::ustring  kActionAudioBackend         {"audio-backend"};
inline const Glib::ustring  kActionAudioDevice          {"audio-device"};
inline const Glib::ustring  kActionAudioDeviceList      {"audio-device-list"};

// Window actions
inline const Glib::ustring kActionShowPrimaryMenu       {"show-primary-menu"};
inline const Glib::ustring kActionShowProfiles          {"show-profiles"};
inline const Glib::ustring kActionShowPreferences       {"show-preferences"};
inline const Glib::ustring kActionShowShortcuts         {"show-shortcuts"};
inline const Glib::ustring kActionShowHelp              {"show-help"};
inline const Glib::ustring kActionShowAbout             {"show-about"};
inline const Glib::ustring kActionShowPendulum          {"show-pendulum"};
inline const Glib::ustring kActionFullScreen            {"full-screen"};

enum class ActionScope {
  kApp,
  kWin
};

struct ActionDescription
{
  ActionScope        scope           {ActionScope::kApp};
  Glib::VariantType  parameter_type  {};
  Glib::VariantBase  initial_state   {};
  Glib::VariantBase  state_hint      {};
  bool               enabled         {true};
};

template<class T>
using ActionStateHintRange = std::tuple<T,T>;

inline constexpr std::size_t kActionStateHintRangeMinimum = 0;
inline constexpr std::size_t kActionStateHintRangeMaximum = 1;

/** Helper to clamp values to the inclusive range of min and max. */
template<class T>
T clampActionStateValue(T value, const ActionStateHintRange<T>& range)
{
  return std::clamp( value,
                     std::get<kActionStateHintRangeMinimum>(range),
                     std::get<kActionStateHintRangeMaximum>(range) );
}

/** Global constants */
inline const      Profile  kDefaultProfile;
inline constexpr  double   kMinimumTempo         = 30;
inline constexpr  double   kMaximumTempo         = 250;
inline constexpr  double   kMinimumTrainerAccel  = 1;
inline constexpr  double   kMaximumTrainerAccel  = 1000;
inline constexpr  double   kDefaultVolume        = 80;
inline constexpr  double   kMinimumVolume        = 0;
inline constexpr  double   kMaximumVolume        = 100;

// number of UTF-8 encoded unicode characters
inline constexpr Glib::ustring::size_type kProfileTitleMaxLength = 255;

// number of UTF-8 encoded unicode characters
inline constexpr Glib::ustring::size_type kProfileDescriptionMaxLength = 1024;

using ProfilesListEntry = std::tuple<Glib::ustring, Glib::ustring, Glib::ustring>;

inline constexpr int kProfilesListEntryIdentifier = 0;
inline constexpr int kProfilesListEntryTitle = 1;
inline constexpr int kProfilesListEntryDescription = 2;

using ProfilesList = std::vector<ProfilesListEntry>;
using ProfilesIdentifierList = std::vector<Glib::ustring>;

using ActionDescriptionMap = std::map<Glib::ustring, ActionDescription>;

inline const ActionDescriptionMap kActionDescriptions =
{
  /* Action         : kActionQuit
   * Scope          : Application
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionQuit, { ActionScope::kApp, {}, {}, {}, true }
  },

  /* Action         : kActionVolume
   * Scope          : Application
   * Parameter type : (gsettings)
   * State type     : (gsettings)
   * State value    : (gsettings)
   * State hint     : -
   * Enabled        : (gsettings)
   */
  { kActionVolume,
    {
      ActionScope::kApp,
      {},
      {},
      {},
      {}
    }
  },

  /* Action         : kActionVolumeIncrease
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionVolumeIncrease,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      {},
      {},
      true
    }
  },

  /* Action         : kActionVolumeDecrease
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionVolumeDecrease,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      {},
      {},
      true
    }
  },

  /* Action         : kActionStart
   * Scope          : Application
   * Parameter type : bool
   * State type     : bool
   * State value    : false
   * State hint     : -
   * Enabled        : true
   */
  { kActionStart,
    {
      ActionScope::kApp,
      {},//Glib::Variant<bool>::variant_type(),
      Glib::Variant<bool>::create( false ),
      {},
      true
    }
  },

  /* Action         : kActionTempo
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.tempo
   * State hint     : (kMinimumTempo,kMaximumTempo)
   * Enabled        : true
   */
  { kActionTempo,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.tempo),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          kMinimumTempo,
          kMaximumTempo
        }),
      true
    }
  },

  /* Action         : kActionTempoIncrease
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionTempoIncrease,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      {},
      {},
      true
    }
  },

  /* Action         : kActionTempoDecrease
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionTempoDecrease,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      {},
      {},
      true
    }
  },

  /* Action         : kActionTempoTap
   * Scope          : Application
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionTempoTap,
    {
      ActionScope::kApp, {}, {}, {}, true
    }
  },

  /* Action         : kActionTrainerEnabled
   * Scope          : Application
   * Parameter type : bool
   * State type     : bool
   * State value    : kDefaultProfile.content.trainer_enabled
   * State hint     : -
   * Enabled        : true
   */
  { kActionTrainerEnabled,
    {
      ActionScope::kApp,
      {}, //Glib::Variant<bool>::variant_type(),
      Glib::Variant<bool>::create(kDefaultProfile.content.trainer_enabled),
      {},
      true
    }
  },

  /* Action         : kActionTrainerStart
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_start
   * State hint     : (kMinimumTempo,kMaximumTempo)
   * Enabled        : true
   */
  { kActionTrainerStart,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_start),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          kMinimumTempo,
          kMaximumTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerTarget
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_target
   * State hint     : (kMinimumTempo,kMaximumTempo)
   * Enabled        : true
   */
  { kActionTrainerTarget,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_target),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          kMinimumTempo,
          kMaximumTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerAccel
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_accel
   * State hint     : (kMinimumTrainerAccel,kMaximumTrainerAccel)
   * Enabled        : true
   */
  { kActionTrainerAccel,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_accel),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          kMinimumTrainerAccel,
          kMaximumTrainerAccel
        }),
      true
    }
  },

  /* Action         : kActionMeterEnabled
   * Scope          : Application
   * Parameter type : bool
   * State type     : bool
   * State value    : kDefaultProfile.content.meter_enabled
   * State hint     : -
   * Enabled        : true
   */
  { kActionMeterEnabled,
    {
      ActionScope::kApp,
      {}, //Glib::Variant<bool>::variant_type(),
      Glib::Variant<bool>::create(kDefaultProfile.content.meter_enabled),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeterSelect,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      Glib::Variant<Glib::ustring>::create(kDefaultProfile.content.meter_select),
      Glib::Variant<std::vector<Glib::ustring>>::create(
        {
          kActionMeter1Simple,
          kActionMeter2Simple,
          kActionMeter3Simple,
          kActionMeter4Simple,
          kActionMeter1Compound,
          kActionMeter2Compound,
          kActionMeter3Compound,
          kActionMeter4Compound,
          kActionMeterCustom
        }),
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter1Simple,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_1_simple),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   *
   */
  { kActionMeter2Simple,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_2_simple),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter3Simple,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_3_simple),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter4Simple,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_4_simple),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter1Compound,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_1_compound),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter2Compound,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_2_compound),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter3Compound,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_3_compound),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeter4Compound,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_4_compound),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionMeterCustom,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_custom),
      {},
      true
    }
  },

  /* Action         : kActionProfilesList
   * Scope          : Application
   * Parameter type : -
   * State type     : ProfilesList
   * State value    : {}
   * State hint     : -
   * Enabled        : true
   */
  { kActionProfilesList,
    {
      ActionScope::kApp,
      {},
      Glib::Variant<ProfilesList>::create({}),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesSelect,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      Glib::Variant<Glib::ustring>::create(""),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesNew, { ActionScope::kApp, {}, {}, {}, true }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesDelete, { ActionScope::kApp, {}, {}, {}, true }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesReset, { ActionScope::kApp, {}, {}, {}, true }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesTitle,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      Glib::Variant<Glib::ustring>::create(""),
      {},
      true
    }
  },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfilesDescription,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      Glib::Variant<Glib::ustring>::create(""),
      {},
      true
    }
  },

  /* Action         : kActionProfilesReorder
   * Scope          : Application
   * Parameter type : ProfilesIdentifierList
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionProfilesReorder,
    {
      ActionScope::kApp,
      Glib::Variant<ProfilesIdentifierList>::variant_type(),
      Glib::Variant<ProfilesIdentifierList>::create({}),
      {},
      true
    }
  },

  /* Action         : kActionShowPrimaryMenu
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowPrimaryMenu,
    { ActionScope::kWin, {}, {}, {}, true }
  },

  /* Action         : kActionShowProfiles
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowProfiles, { ActionScope::kWin, {}, {}, {}, true }
  },

  /* Action         : kActionShowPreferences
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowPreferences, { ActionScope::kWin, {}, {}, {}, true }
  },

  /* Action         : kActionShowShortcuts
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowShortcuts, { ActionScope::kWin, {}, {}, {}, true }
  },

  /* Action         : kActionShowHelp
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowHelp, { ActionScope::kWin, {}, {}, {}, false }
  },

  /* Action         : kActionShowAbout
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowAbout, { ActionScope::kWin, {}, {}, {}, true }
  },

  /* Action         : kActionShowPendulum
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowPendulum,
    {
      ActionScope::kWin,
      {},//Glib::Variant<bool>::variant_type(),
      Glib::Variant<bool>::create( true ),
      {},
      true
    }
  },

  /* Action         : kActionFullScreen
   * Scope          : Window
   * Parameter type : bool
   * State type     : bool
   * State value    : false
   * State hint     : -
   * Enabled        : true
   */
  { kActionFullScreen,
    {
      ActionScope::kWin,
      {},//Glib::Variant<bool>::variant_type(),
      Glib::Variant<bool>::create( false ),
      {},
      true
    }
  },

  /* Action         : kActionAudioDeviceList
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionAudioDeviceList,
    {
      ActionScope::kApp,
      Glib::Variant<std::vector<Glib::ustring>>::variant_type(),
      Glib::Variant<std::vector<Glib::ustring>>::create({}),
      {},
      true
    }
  }
};

// There are two posibilities to setup and install actions:
//
// 1) ActionHandlerSlot:     Creates a Gio::SimpleAction according to the parameters given
//                           in ActionDescription; connects the given handler slot to
//                           signal_activate() or signal_change_state() and adds the action
//                           to the given Gio::ActionMap
//
// 2) ActionHandlerSettings: Uses Gio::Settings::create_action method to create the action
//                           and adds it to the given Gio::ActionMap.
//                           See glibmm documentation for details.
//
using ActionHandlerSlot     = sigc::slot<void, const Glib::VariantBase&>;
using ActionHandlerSettings = Glib::RefPtr<Gio::Settings>;
using ActionHandler         = std::variant<ActionHandlerSlot, ActionHandlerSettings>;

// map action names to action handler
using ActionHandlerMap      = std::map<Glib::ustring, ActionHandler>;

void install_action(Gio::ActionMap& gmap,
                    const Glib::ustring action_name,
                    const ActionDescription& action_descr,
                    const ActionHandler& handler);

void install_actions(Gio::ActionMap& gmap,
                     const ActionDescriptionMap& descriptions,
                     const ActionHandlerMap& handler);


#endif//GMetronome_Action_h
