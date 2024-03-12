/*
 * Copyright (C) 2020-2024 The GMetronome Team
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

#include "Action.h"

const ActionDescriptionMap kActionDescriptions =
{
  /* Action         : kActionQuit
   * Scope          : Application
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionQuit, { ActionScope::kApp, {}, {}, {}, true } },

  /* Action         : kActionVolume
   * Scope          : Application
   * Parameter type : (gsettings)
   * State type     : (gsettings)
   * State value    : (gsettings)
   * State hint     : -
   * Enabled        : (gsettings)
   */
  { kActionVolume, { ActionScope::kApp, {}, {}, {}, {} } },

  /* Action         : kActionVolumeChange
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionVolumeChange,
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
   * State hint     : (Profile::kMinTempo,Profile::kMaxTempo)
   * Enabled        : true
   */
  { kActionTempo,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.tempo),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinTempo,
          Profile::kMaxTempo
        }),
      true
    }
  },

  /* Action         : kActionTempoChange
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionTempoChange,
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
  { kActionTempoTap, { ActionScope::kApp, {}, {}, {}, true } },

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
   * State hint     : (Profile::kMinTempo,Profile::kMaxTempo)
   * Enabled        : true
   */
  { kActionTrainerStart,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_start),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinTempo,
          Profile::kMaxTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerTarget
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_target
   * State hint     : (Profile::kMinTempo,Profile::kMaxTempo)
   * Enabled        : true
   */
  { kActionTrainerTarget,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_target),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinTempo,
          Profile::kMaxTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerAccel
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_accel
   * State hint     : (Profile::kMinTrainerAccel,Profile::kMaxTrainerAccel)
   * Enabled        : true
   */
  { kActionTrainerAccel,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_accel),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinTrainerAccel,
          Profile::kMaxTrainerAccel
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

  /* Action         : kActionMeterSelect
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
          kActionMeterSimple2,
          kActionMeterSimple3,
          kActionMeterSimple4,
          kActionMeterCompound2,
          kActionMeterCompound3,
          kActionMeterCompound4,
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
   *
   */
  { kActionMeterSimple2,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_simple_2),
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
  { kActionMeterSimple3,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_simple_3),
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
  { kActionMeterSimple4,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_simple_4),
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
  { kActionMeterCompound2,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_compound_2),
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
  { kActionMeterCompound3,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_compound_3),
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
  { kActionMeterCompound4,
    {
      ActionScope::kApp,
      Glib::Variant<Meter>::variant_type(),
      Glib::Variant<Meter>::create(kDefaultProfile.content.meter_compound_4),
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

  /* Action         : kActionMeterSeek
   * Scope          : Application
   * Parameter type : double
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionMeterSeek,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      {},
      {},
      true
    }
  },

  /* Action         : kActionProfileList
   * Scope          : Application
   * Parameter type : -
   * State type     : ProfileList
   * State value    : {}
   * State hint     : -
   * Enabled        : true
   */
  { kActionProfileList,
    {
      ActionScope::kApp,
      {},
      Glib::Variant<ProfileList>::create({}),
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
  { kActionProfileSelect,
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
  { kActionProfileNew,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      {},
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
  { kActionProfileDelete, { ActionScope::kApp, {}, {}, {}, true } },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfileReset, { ActionScope::kApp, {}, {}, {}, true } },

  /* Action         :
   * Scope          : Application
   * Parameter type :
   * State type     :
   * State value    :
   * State hint     :
   * Enabled        :
   */
  { kActionProfileTitle,
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
  { kActionProfileDescription,
    {
      ActionScope::kApp,
      Glib::Variant<Glib::ustring>::variant_type(),
      Glib::Variant<Glib::ustring>::create(""),
      {},
      true
    }
  },

  /* Action         : kActionProfileReorder
   * Scope          : Application
   * Parameter type : ProfileIdentifierList
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionProfileReorder,
    {
      ActionScope::kApp,
      Glib::Variant<ProfileIdentifierList>::variant_type(),
      Glib::Variant<ProfileIdentifierList>::create({}),
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
  },

  /* Action         : kActionShowPrimaryMenu
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowPrimaryMenu, { ActionScope::kWin, {}, {}, {}, true } },

  /* Action         : kActionShowProfiles
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowProfiles, { ActionScope::kWin, {}, {}, {}, true } },

  /* Action         : kActionShowPreferences
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowPreferences, { ActionScope::kWin, {}, {}, {}, true } },

  /* Action         : kActionShowShortcuts
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowShortcuts, { ActionScope::kWin, {}, {}, {}, true } },

  /* Action         : kActionShowHelp
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowHelp, { ActionScope::kWin, {}, {}, {}, false } },

  /* Action         : kActionShowAbout
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionShowAbout, { ActionScope::kWin, {}, {}, {}, true } },

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

  /* Action         : kActionPendulumTogglePhase
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionPendulumTogglePhase, { ActionScope::kWin, {}, {}, {}, true } },

  /* Action         : kActionTempoQuickSet
   * Scope          : Window
   * Parameter type : -
   * State type     : -
   * State value    : -
   * State hint     : -
   * Enabled        : true
   */
  { kActionTempoQuickSet, { ActionScope::kWin, {}, {}, {}, true } },
};

namespace {

  Glib::RefPtr<Gio::SimpleAction>
  create_simple_action(const Glib::ustring& action_name,
                       const ActionDescription& action_descr)
  {
    Glib::RefPtr<Gio::SimpleAction> action = Gio::SimpleAction::create(
      action_name,
      action_descr.parameter_type,
      action_descr.initial_state );

    if (action_descr.state_hint)
      action->set_state_hint(action_descr.state_hint);

    action->set_enabled(action_descr.enabled);

    return action;
  }

  Glib::RefPtr<Gio::SimpleAction>
  create_simple_action(const Glib::ustring& action_name,
                       const ActionDescription& action_descr,
                       const ActionHandlerSlot& action_slot)
  {
    Glib::RefPtr<Gio::SimpleAction> action = create_simple_action(action_name, action_descr);

    if (action && action_slot.has_value())
    {
      const auto& slot = action_slot.value();

      if (action->get_state_variant())
        action->signal_change_state().connect(slot);
      else // stateless action
        action->signal_activate().connect(slot);
    }
    return action;
  }

  Glib::RefPtr<Gio::Action>
  create_settings_action(const Glib::ustring& action_name,
                         const ActionHandlerSlot& action_slot,
                         Glib::RefPtr<Gio::Settings> settings)
  {
    Glib::RefPtr<Gio::Action> action;
    if (settings)
    {
      if (action = settings->create_action(action_name); action)
      {
        if (action_slot.has_value()) {
          const auto& slot = action_slot.value();

          action->property_state().signal_changed().connect(
            [action, slot] () { slot(action->get_state_variant()); });
        }
      }
    }
    return action;
  }

  void install_action(Gio::ActionMap& action_map,
                      const Glib::ustring action_name,
                      const ActionDescription& action_descr,
                      const ActionHandlerSlot& action_slot,
                      Glib::RefPtr<Gio::Settings> settings)
  {
    Glib::RefPtr<Gio::Action> action;

    if (settings)
      action = create_settings_action(action_name, action_slot, settings);
    else
      action = create_simple_action(action_name, action_descr, action_slot);

    if (action)
      action_map.add_action(action);
  }

}//unnamed namespace

void install_actions(Gio::ActionMap& action_map, const ActionHandlerList& handler)
{
  for (const auto& [action_name, action_slot, settings] : handler)
  {
    if (auto action_descr_it = kActionDescriptions.find(action_name);
        action_descr_it != kActionDescriptions.end())
    {
      install_action(action_map, action_name, action_descr_it->second, action_slot, settings);
    }
  }
}
