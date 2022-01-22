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
   * State hint     : (Profile::kMinimumTempo,Profile::kMaximumTempo)
   * Enabled        : true
   */
  { kActionTempo,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.tempo),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinimumTempo,
          Profile::kMaximumTempo
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
   * State hint     : (Profile::kMinimumTempo,Profile::kMaximumTempo)
   * Enabled        : true
   */
  { kActionTrainerStart,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_start),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinimumTempo,
          Profile::kMaximumTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerTarget
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_target
   * State hint     : (Profile::kMinimumTempo,Profile::kMaximumTempo)
   * Enabled        : true
   */
  { kActionTrainerTarget,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_target),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinimumTempo,
          Profile::kMaximumTempo
        }),
      true
    }
  },

  /* Action         : kActionTrainerAccel
   * Scope          : Application
   * Parameter type : double
   * State type     : double
   * State value    : kDefaultProfile.content.trainer_accel
   * State hint     : (Profile::kMinimumTrainerAccel,Profile::kMaximumTrainerAccel)
   * Enabled        : true
   */
  { kActionTrainerAccel,
    {
      ActionScope::kApp,
      Glib::Variant<double>::variant_type(),
      Glib::Variant<double>::create(kDefaultProfile.content.trainer_accel),
      Glib::Variant<ActionStateHintRange<double>>::create(
        {
          Profile::kMinimumTrainerAccel,
          Profile::kMaximumTrainerAccel
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

namespace {

  Glib::RefPtr<Gio::SimpleAction> create_custom_action(const Glib::ustring& action_name,
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

}//unnamed namespace

void install_action(Gio::ActionMap& gmap,
                    const Glib::ustring action_name,
                    const ActionDescription& action_descr,
                    const ActionHandler& action_handler)
{
  Glib::RefPtr<Gio::Action> action;

  if (std::holds_alternative<ActionHandlerSlot>(action_handler))
  {
    auto& slot = std::get<ActionHandlerSlot>(action_handler);

    Glib::RefPtr<Gio::SimpleAction> simple_action =
      create_custom_action(action_name, action_descr);

    if ( simple_action->get_state_variant() )
      simple_action->signal_change_state().connect(slot);
    else // stateless action
      simple_action->signal_activate().connect(slot);

    action = simple_action;
  }
  else if (std::holds_alternative<ActionHandlerSettings>(action_handler))
  {
    auto& settings = std::get<ActionHandlerSettings>(action_handler);
    if (settings)
      action = settings->create_action(action_name);
  }

  if (action) gmap.add_action(action);
}

void install_actions(Gio::ActionMap& gmap,
                     const ActionDescriptionMap& descriptions,
                     const ActionHandlerMap& handler)
{
  for (const auto& [action_name, action_handler] : handler)
  {
    if (auto action_descr_it = descriptions.find(action_name);
        action_descr_it != descriptions.end())
    {
      install_action(gmap, action_name, action_descr_it->second, action_handler);
    }
  }
}
