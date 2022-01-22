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

// Global constants
inline constexpr  double   kDefaultVolume        = 80;
inline constexpr  double   kMinimumVolume        = 0;
inline constexpr  double   kMaximumVolume        = 100;

enum class ActionScope
{
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

// helper to clamp values to the inclusive range of min and max
template<class T>
T clampActionStateValue(T value, const ActionStateHintRange<T>& range)
{
  return std::clamp( value,
                     std::get<kActionStateHintRangeMinimum>(range),
                     std::get<kActionStateHintRangeMaximum>(range) );
}

using ProfilesListEntry = std::tuple<Glib::ustring, Glib::ustring, Glib::ustring>;

inline constexpr int kProfilesListEntryIdentifier = 0;
inline constexpr int kProfilesListEntryTitle = 1;
inline constexpr int kProfilesListEntryDescription = 2;

using ProfilesList = std::vector<ProfilesListEntry>;
using ProfilesIdentifierList = std::vector<Glib::ustring>;

using ActionDescriptionMap = std::map<Glib::ustring, ActionDescription>;

extern const ActionDescriptionMap kActionDescriptions;

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
