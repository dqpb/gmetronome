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

  GType getGTypeForVariantType(const Glib::VariantType& variant_type)
  {
    GType type;
    auto var_type_char = g_variant_type_peek_string(variant_type.gobj())[0];

    switch (var_type_char)
    {
    case G_VARIANT_CLASS_BOOLEAN:     type = G_TYPE_BOOLEAN; break;
    case G_VARIANT_CLASS_BYTE:        type = G_TYPE_UCHAR;   break;
    case G_VARIANT_CLASS_INT16:       type = G_TYPE_INT;     break;
    case G_VARIANT_CLASS_UINT16:      type = G_TYPE_UINT;    break;
    case G_VARIANT_CLASS_INT32:       type = G_TYPE_INT;     break;
    case G_VARIANT_CLASS_UINT32:      type = G_TYPE_UINT;    break;
    case G_VARIANT_CLASS_INT64:       type = G_TYPE_INT64;   break;
    case G_VARIANT_CLASS_UINT64:      type = G_TYPE_UINT64;  break;
    case G_VARIANT_CLASS_DOUBLE:      type = G_TYPE_DOUBLE;  break;
    case G_VARIANT_CLASS_STRING:      type = G_TYPE_STRING;  break;
    case G_VARIANT_CLASS_OBJECT_PATH: type = G_TYPE_STRING;  break;
    case G_VARIANT_CLASS_SIGNATURE:   type = G_TYPE_STRING;  break;

    // currently unsupported types
    case G_VARIANT_CLASS_ARRAY:       [[fallthrough]];
    case G_VARIANT_CLASS_HANDLE:      [[fallthrough]];
    case G_VARIANT_CLASS_VARIANT:     [[fallthrough]];
    case G_VARIANT_CLASS_MAYBE:       [[fallthrough]];
    case G_VARIANT_CLASS_TUPLE:       [[fallthrough]];
    case G_VARIANT_CLASS_DICT_ENTRY:  [[fallthrough]];
    default:
      type = G_TYPE_NONE;
      break;
    };

    return type;
  }

  GType getPropertyType(const Glib::PropertyProxy_Base& prop)
  {
    GObjectClass* prop_object_class = G_OBJECT_GET_CLASS(prop.get_object()->gobj());
    GParamSpec* prop_spec = g_object_class_find_property(prop_object_class, prop.get_name());
    return G_PARAM_SPEC_VALUE_TYPE(prop_spec);
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
  GType property_type = getPropertyType(property_);

  property_value_.init(property_type);

  action_param_type_ = action_group_->get_action_parameter_type(action_name_);

  GType action_param_value_type = getGTypeForVariantType(action_param_type_);

  if (action_param_value_type == G_TYPE_NONE)
    throw std::runtime_error(
      "ActionBinding: GValue to GVariant conversion not implemented for this type.");

  action_param_value_.init(action_param_value_type);

  bool is_compatible = g_value_type_compatible(property_type, action_param_value_type)
    && g_value_type_compatible(action_param_value_type, property_type);

  bool is_transformable = g_value_type_transformable(property_type, action_param_value_type)
    && g_value_type_transformable(action_param_value_type, property_type);

  if (!is_compatible && !is_transformable)
    throw std::runtime_error("ActionBinding: GValue types not transformable.");

  need_transform_ = !is_compatible;

  action_connection_ = action_group_->signal_action_state_changed(action_name_)
    .connect(sigc::mem_fun(*this, &ActionBinding::onActionStateChanged));

  prop_connection_ = property_.signal_changed()
    .connect(sigc::mem_fun(*this, &ActionBinding::onPropertyValueChanged));

  onActionStateChanged(action_name_, action_group_->get_action_state_variant(action_name_));
}

void ActionBinding::onActionStateChanged(const Glib::ustring& action_name,
                                         const Glib::VariantBase& variant)
{
  g_dbus_gvariant_to_gvalue(const_cast<GVariant*>(variant.gobj()),
                            action_param_value_.gobj());
  if (need_transform_)
    g_value_transform(action_param_value_.gobj(), property_value_.gobj());
  else
    property_value_ = action_param_value_;

  prop_connection_.block();
  property_.get_object()->set_property_value(property_.get_name(), property_value_);
  prop_connection_.unblock();
}

void ActionBinding::onPropertyValueChanged()
{
  property_.get_object()->get_property_value(property_.get_name(), property_value_);

  if (need_transform_)
    g_value_transform(property_value_.gobj(), action_param_value_.gobj());
  else
    action_param_value_ = property_value_;

  Glib::VariantBase param = Glib::wrap(
    g_dbus_gvalue_to_gvariant(action_param_value_.gobj(), action_param_type_.gobj())
  );

  action_connection_.block();
  action_group_->activate_action(action_name_, param);
  action_connection_.unblock();
}

Glib::RefPtr<ActionBinding> bind_action(Glib::RefPtr<Gio::ActionGroup> action_group,
                                        const Glib::ustring& action_name,
                                        Glib::PropertyProxy_Base property)
{
  return ActionBinding::create(action_group, action_name, property);
}
