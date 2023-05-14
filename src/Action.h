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

using ProfileListEntry = std::tuple<Glib::ustring, Glib::ustring, Glib::ustring>;

inline constexpr int kProfileListEntryIdentifier = 0;
inline constexpr int kProfileListEntryTitle = 1;
inline constexpr int kProfileListEntryDescription = 2;

using ProfileList = std::vector<ProfileListEntry>;
using ProfileIdentifierList = std::vector<Glib::ustring>;

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
using ActionHandlerMap = std::map<Glib::ustring, ActionHandler>;

void install_action(Gio::ActionMap& gmap,
                    const Glib::ustring action_name,
                    const ActionDescription& action_descr,
                    const ActionHandler& handler);

void install_actions(Gio::ActionMap& gmap,
                     const ActionDescriptionMap& descriptions,
                     const ActionHandlerMap& handler);

#endif//GMetronome_Action_h
