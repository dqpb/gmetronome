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
