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

#ifndef GMetronome_ActionBinding_h
#define GMetronome_ActionBinding_h

#include <gtkmm.h>

class ActionBinding : public Glib::Object
{
public:
  // noncopyable
  ActionBinding(const ActionBinding&) = delete;
  ActionBinding& operator=(const ActionBinding&) = delete;

  static Glib::RefPtr<ActionBinding> create(Glib::RefPtr<Gio::ActionGroup> action_group,
                                            const Glib::ustring& action_name,
                                            Glib::PropertyProxy_Base property);

protected:
  ActionBinding(Glib::RefPtr<Gio::ActionGroup> action_group,
                const Glib::ustring& action_name,
                Glib::PropertyProxy_Base property);

private:
  sigc::connection action_connection_;
  sigc::connection prop_connection_;
  Glib::RefPtr<Gio::ActionGroup> action_group_;
  Glib::ustring action_name_;
  Glib::PropertyProxy_Base property_;
  Glib::ValueBase property_value_;
  Glib::VariantType action_param_type_;
  Glib::ValueBase action_param_value_;
  bool need_transform_ {false};

  void onActionStateChanged(const Glib::ustring& action_name,
                            const Glib::VariantBase& variant);

  void onPropertyValueChanged();
};


Glib::RefPtr<ActionBinding> bind_action(Glib::RefPtr<Gio::ActionGroup> action_group,
                                        const Glib::ustring& action_name,
                                        Glib::PropertyProxy_Base property);
template<class T>
Glib::RefPtr<ActionBinding> bind_action(Glib::RefPtr<Gio::ActionGroup> action_group,
                                        const Glib::ustring& action_name,
                                        Glib::Property<T>& property)
{ return bind_action(action_group, action_name, property.get_proxy()); }

#endif//GMetronome_ActionBinding_h
