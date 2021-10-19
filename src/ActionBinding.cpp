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

#include "ActionBinding.h"
#include <stdexcept>
#include <iostream>

namespace {

  Glib::ValueBase variant_to_value(const Glib::VariantBase& variant)
  {
    Glib::ValueBase value;
    g_dbus_gvariant_to_gvalue(const_cast<GVariant*>(variant.gobj()), value.gobj());    
    return value;
  }

  Glib::VariantBase value_to_variant(const Glib::ValueBase& value)
  {
    const GValue* gvalue = value.gobj();
    const GVariantType* type = nullptr;
    
    switch (G_VALUE_TYPE (gvalue) )
    {
    case G_TYPE_DOUBLE:
    case G_TYPE_FLOAT:
      type = G_VARIANT_TYPE_DOUBLE;
      break;      
    case G_TYPE_UCHAR:
    case G_TYPE_CHAR:
      type = G_VARIANT_TYPE_BYTE;
      break;
    case G_TYPE_BOOLEAN:
      type = G_VARIANT_TYPE_BOOLEAN;
      break;
    case G_TYPE_INT:
      type = G_VARIANT_TYPE_INT32;
      break;
    case G_TYPE_UINT:
      type = G_VARIANT_TYPE_UINT32;
        break;
    case G_TYPE_LONG:
      type = G_VARIANT_TYPE_INT64;
      break;
    case G_TYPE_ULONG:
      type = G_VARIANT_TYPE_UINT64;
      break;
    case G_TYPE_INT64:
      type = G_VARIANT_TYPE_INT64;
      break;
    case G_TYPE_UINT64:
      type = G_VARIANT_TYPE_UINT64;
      break;
    case G_TYPE_STRING:
      type = G_VARIANT_TYPE_STRING;
      break;
    case G_TYPE_VARIANT:
      type = G_VARIANT_TYPE_VARIANT;
      break;
      
    // case G_TYPE_GTYPE:
    //   break;
    // case G_TYPE_CHECKSUM:
    //   break;
    case G_TYPE_POINTER:
    case G_TYPE_BOXED:
    case G_TYPE_PARAM:
    case G_TYPE_OBJECT:
    case G_TYPE_ENUM:
    case G_TYPE_FLAGS:
    case G_TYPE_INVALID:
    case G_TYPE_NONE:
    case G_TYPE_INTERFACE:
    default:
      throw std::runtime_error("GValue to GVariant conversion not implemented for this type.");
      break;
    };
    
    return Glib::wrap(g_dbus_gvalue_to_gvariant(gvalue, type));
  }
  
}//unnamed namespace

Glib::RefPtr<ActionBinding> ActionBinding::create(Glib::RefPtr<Gio::ActionGroup> action_group,
						  const Glib::ustring& action_name,
                                                  Glib::PropertyProxy_Base property)
{
  return Glib::RefPtr<ActionBinding>( new ActionBinding(action_group, action_name, property) );
}

ActionBinding::ActionBinding(Glib::RefPtr<Gio::ActionGroup> action_group,
			     const Glib::ustring& action_name,
                             Glib::PropertyProxy_Base property)
  : action_group_(action_group),
    action_name_(action_name),
    property_(property)
{

#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 60
  GObjectClass* property_object_class = G_OBJECT_GET_CLASS(property_.get_object()->gobj());

  GParamSpec* prop_spec = g_object_class_find_property(property_object_class,
						       property_.get_name());

  property_value_.init(G_PARAM_SPEC_VALUE_TYPE(prop_spec));
#endif    
  
  action_connection_ = action_group_->signal_action_state_changed(action_name_)
    .connect(sigc::mem_fun(*this, &ActionBinding::onActionStateChanged));

  prop_connection_ = property_.signal_changed()
    .connect(sigc::mem_fun(*this, &ActionBinding::onPropertyValueChanged));

  onActionStateChanged(action_name_, action_group_->get_action_state_variant(action_name_));
}

void ActionBinding::onActionStateChanged(const Glib::ustring& action_name,
					 const Glib::VariantBase& variant)
{
  std::cout << G_STRFUNC << std::endl;
  prop_connection_.block();
  property_.get_object()->set_property_value(property_.get_name(),
                                             variant_to_value(variant));
  prop_connection_.unblock();
}

void ActionBinding::onPropertyValueChanged()
{
  std::cout << G_STRFUNC << std::endl;
  
  property_.get_object()->get_property_value(property_.get_name(), property_value_);

  action_connection_.block();
  action_group_->activate_action(action_name_, value_to_variant(property_value_));
  action_connection_.unblock();
}



Glib::RefPtr<ActionBinding> bind_action(Glib::RefPtr<Gio::ActionGroup> action_group,
					const Glib::ustring& action_name,
                                        Glib::PropertyProxy_Base property)
{
  return ActionBinding::create(action_group, action_name, property);
}


