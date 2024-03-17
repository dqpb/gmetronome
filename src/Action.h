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

#ifndef GMetronome_Action_h
#define GMetronome_Action_h

#include "Profile.h"
#include "Meter.h"

#include <giomm.h>
#include <utility>
#include <tuple>
#include <vector>
#include <map>
#include <variant>
#include <functional>
#include <optional>

// Application actions
inline const Glib::ustring  kActionQuit                 {"quit"};
inline const Glib::ustring  kActionStart                {"start"};
inline const Glib::ustring  kActionVolume               {"volume"};
inline const Glib::ustring  kActionVolumeChange         {"volume-change"};
inline const Glib::ustring  kActionVolumeMute           {"volume-mute"};
inline const Glib::ustring  kActionTempo                {"tempo"};
inline const Glib::ustring  kActionTempoChange          {"tempo-change"};
inline const Glib::ustring  kActionTempoTap             {"tempo-tap"};
inline const Glib::ustring  kActionTrainerEnabled       {"trainer-enabled"};
inline const Glib::ustring  kActionTrainerStart         {"trainer-start"};
inline const Glib::ustring  kActionTrainerTarget        {"trainer-target"};
inline const Glib::ustring  kActionTrainerAccel         {"trainer-accel"};
inline const Glib::ustring  kActionMeterEnabled         {"meter-enabled"};
inline const Glib::ustring  kActionMeterSelect          {"meter-select"};
inline const Glib::ustring  kActionMeterSimple2         {"meter-simple-2"};
inline const Glib::ustring  kActionMeterSimple3         {"meter-simple-3"};
inline const Glib::ustring  kActionMeterSimple4         {"meter-simple-4"};
inline const Glib::ustring  kActionMeterCompound2       {"meter-compound-2"};
inline const Glib::ustring  kActionMeterCompound3       {"meter-compound-3"};
inline const Glib::ustring  kActionMeterCompound4       {"meter-compound-4"};
inline const Glib::ustring  kActionMeterCustom          {"meter-custom"};
inline const Glib::ustring  kActionMeterSeek            {"meter-seek"};
inline const Glib::ustring  kActionProfileList          {"profile-list"};
inline const Glib::ustring  kActionProfileSelect        {"profile-select"};
inline const Glib::ustring  kActionProfileNew           {"profile-new"};
inline const Glib::ustring  kActionProfileDelete        {"profile-delete"};
inline const Glib::ustring  kActionProfileReset         {"profile-reset"};
inline const Glib::ustring  kActionProfileTitle         {"profile-title"};
inline const Glib::ustring  kActionProfileDescription   {"profile-description"};
inline const Glib::ustring  kActionProfileReorder       {"profile-reorder"};
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
inline const Glib::ustring kActionTempoQuickSet         {"tempo-quick-set"};
inline const Glib::ustring kActionPendulumTogglePhase   {"pendulum-toggle-phase"};

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

constexpr std::size_t kActionStateHintRangeMinimum = 0;
constexpr std::size_t kActionStateHintRangeMaximum = 1;

// helper to clamp values to the inclusive range of min and max
template<class T>
T clampActionStateValue(T value, const ActionStateHintRange<T>& range)
{
  return std::clamp( value,
                     std::get<kActionStateHintRangeMinimum>(range),
                     std::get<kActionStateHintRangeMaximum>(range) );
}

using ProfileListEntry = std::tuple<Glib::ustring, Glib::ustring, Glib::ustring>;

constexpr int kProfileListEntryIdentifier = 0;
constexpr int kProfileListEntryTitle = 1;
constexpr int kProfileListEntryDescription = 2;

using ProfileList = std::vector<ProfileListEntry>;
using ProfileIdentifierList = std::vector<Glib::ustring>;

using ActionDescriptionMap = std::map<Glib::ustring, ActionDescription>;

extern const ActionDescriptionMap kActionDescriptions;

// There are two posibilities to setup and install actions:
//
// 1) simple action:         Creates a Gio::SimpleAction according to the parameters given
//                           in kActionDescriptions; connects the given handler slot to
//                           signal_activate() or signal_change_state() and adds the action
//                           to the given Gio::ActionMap.
//
// 2) settings action:       Uses the given Gio::Settings object and calls its create_action
//                           method to build a Gio::Action. This action is added to the given
//                           Gio::ActionMap. The slot is called whenever the state property
//                           (i.e. the value of the settings key) of the action changes.
//                           In this case the optional slot does not actually define the
//                           action logic but is a callback to get informed whenever the
//                           value of a settings key changes.

using ActionHandlerSlot = std::optional<sigc::slot<void(const Glib::VariantBase&)>>;

inline const ActionHandlerSlot kActionNoSlot;
inline const ActionHandlerSlot kActionNullSlot {std::in_place};
inline const ActionHandlerSlot kActionEmptySlot {std::in_place, [](const Glib::VariantBase&){}};

struct ActionHandlerListEntry
{
  Glib::ustring action_name;
  ActionHandlerSlot slot;
  Glib::RefPtr<Gio::Settings> settings;
};

using ActionHandlerList = std::vector<ActionHandlerListEntry>;

void install_actions(Gio::ActionMap& action_map, const ActionHandlerList& handler);

#endif//GMetronome_Action_h
