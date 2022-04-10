/*
 * Copyright (C) 2022 The GMetronome Team
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "SoundThemeEditor.h"
#include "Settings.h"
#include <glibmm/i18n.h>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

SoundThemeEditor::SoundThemeEditor(BaseObjectType* obj,
                                   const Glib::RefPtr<Gtk::Builder>& builder,
                                   Glib::ustring theme_id)
  : Glib::ObjectBase("SoundThemeEditor"),
    Gtk::Window(obj),
    builder_(builder),
    theme_id_{std::move(theme_id)}
{
  builder_->get_widget("mainBox", main_box_);
  builder_->get_widget("parametersFrame", parameters_frame_);
  builder_->get_widget("titleEntry", title_entry_);
  builder_->get_widget("strongRadioButton", strong_radio_button_);
  builder_->get_widget("midRadioButton", mid_radio_button_);
  builder_->get_widget("weakRadioButton", weak_radio_button_);
  builder_->get_widget("parametersGrid", parameters_grid_);
  builder_->get_widget("bellSwitch", bell_switch_);
  builder_->get_widget("balanceScale", balance_scale_);
  builder_->get_widget("unavailableLabel", unavailable_label_);

  timbre_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("timbreAdjustment"));
  pitch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("pitchAdjustment"));
  bell_volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("bellVolumeAdjustment"));
  balance_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("balanceAdjustment"));
  volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("volumeAdjustment"));

  strong_accent_drawing_.setAccentState(kAccentStrong);
  mid_accent_drawing_.setAccentState(kAccentMid);
  weak_accent_drawing_.setAccentState(kAccentWeak);

  strong_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);
  mid_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);
  weak_accent_drawing_.set_valign(Gtk::ALIGN_CENTER);

  strong_accent_drawing_.show();
  mid_accent_drawing_.show();
  weak_accent_drawing_.show();

  strong_radio_button_->add(strong_accent_drawing_);
  mid_radio_button_->add(mid_accent_drawing_);
  weak_radio_button_->add(weak_accent_drawing_);

  balance_scale_->add_mark(0.0, Gtk::POS_BOTTOM, "");

  signal_key_press_event()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onKeyPressEvent));

  strong_radio_button_->signal_clicked().connect(
    [this] () { if (strong_radio_button_->get_active()) updateThemeBindings(); });
  mid_radio_button_->signal_clicked().connect(
    [this] () { if (mid_radio_button_->get_active()) updateThemeBindings(); });
  weak_radio_button_->signal_clicked().connect(
    [this] () { if (weak_radio_button_->get_active()) updateThemeBindings(); });

  settings::soundThemeList()->settings()->signal_changed()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onSettingsListChanged));

  updateThemeBindings();
}

SoundThemeEditor::~SoundThemeEditor()
{
  // nothing to do
}

//static
SoundThemeEditor* SoundThemeEditor::create(Gtk::Window& parent, Glib::ustring theme_id)
{
  // load the Builder file and instantiate the dialog
  auto resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/ui/SoundThemeEditor.glade";
  auto builder = Gtk::Builder::create_from_resource(resource_path);

  SoundThemeEditor* dialog = nullptr;
  builder->get_widget_derived("editorWindow", dialog, std::move(theme_id));
  if (!dialog)
    throw std::runtime_error("No \"editorWindow\" object in SoundThemeEditor.glade");

  dialog->set_transient_for(parent);
  return dialog;
}

bool SoundThemeEditor::onKeyPressEvent(GdkEventKey* event)
{
  switch (event->keyval) {
  case GDK_KEY_Escape:
  {
    close();
    return TRUE;
  }
  default:
    return FALSE;
  };
}

template<int index>
gboolean soundParamsGetMapping(GValue *gval, GVariant *gvar, gpointer user_data)
{
  using ParamValueType = std::tuple_element_t<index, settings::SoundParametersTuple>;

  settings::SoundParametersTuple& current_params =
    *static_cast<settings::SoundParametersTuple*>(user_data);

  if (gvar)
  {
    Glib::Variant<settings::SoundParametersTuple> gvar_wrap(gvar, true);

    ParamValueType new_param_value = gvar_wrap.get_child<ParamValueType>(index);
    std::get<index>(current_params) = new_param_value;

    Glib::Value<ParamValueType> gval_tmp;
    gval_tmp.init(G_VALUE_TYPE(gval));
    gval_tmp.set(new_param_value);
    g_value_copy(gval_tmp.gobj(), gval);

    return true;
  }
  else return false;
}

template<int index>
GVariant* soundParamsSetMapping(const GValue *gval,
                                const GVariantType *expected_type,
                                gpointer user_data)
{
  using ParamValueType = std::tuple_element_t<index, settings::SoundParametersTuple>;

  settings::SoundParametersTuple& params =
    *static_cast<settings::SoundParametersTuple*>(user_data);

  Glib::Value<ParamValueType> gval_tmp;
  gval_tmp.init(gval);

  std::get<index>(params) = gval_tmp.get();

  auto var_tmp = Glib::Variant<settings::SoundParametersTuple>::create(params);
  return var_tmp.gobj_copy();
}

void unbindProperty(const Glib::PropertyProxy_Base& p)
{
  g_settings_unbind(p.get_object()->gobj(), p.get_name());
}

template<int index>
void bindSoundParameter(Glib::RefPtr<Gio::Settings> settings, // theme settings
                        const Glib::ustring& key,             // params settings key
                        const Glib::PropertyProxy_Base& property_proxy,
                        settings::SoundParametersTuple* params)
{
  g_settings_bind_with_mapping (settings->gobj(),
                                key.c_str(),
                                property_proxy.get_object()->gobj(),
                                property_proxy.get_name(),
                                G_SETTINGS_BIND_DEFAULT,
                                soundParamsGetMapping<index>,
                                soundParamsSetMapping<index>,
                                params,
                                nullptr);
}

void SoundThemeEditor::bindProperties(Glib::RefPtr<Gio::Settings> settings,
                                      const Glib::ustring& key)
{
  settings->bind(settings::kKeySoundThemeTitle, title_entry_->property_text());

  bindSoundParameter<0>(settings, key, timbre_adjustment_->property_value(), &sound_params_);
  bindSoundParameter<1>(settings, key, pitch_adjustment_->property_value(), &sound_params_);
  bindSoundParameter<2>(settings, key, bell_switch_->property_state(), &sound_params_);
  bindSoundParameter<3>(settings, key, bell_volume_adjustment_->property_value(), &sound_params_);
  bindSoundParameter<4>(settings, key, balance_adjustment_->property_value(), &sound_params_);
  bindSoundParameter<5>(settings, key, volume_adjustment_->property_value(), &sound_params_);
}

void SoundThemeEditor::unbindProperties()
{
  unbindProperty(title_entry_->property_text());

  unbindProperty(timbre_adjustment_->property_value());
  unbindProperty(pitch_adjustment_->property_value());
  unbindProperty(bell_switch_->property_state());
  unbindProperty(bell_volume_adjustment_->property_value());
  unbindProperty(balance_adjustment_->property_value());
  unbindProperty(volume_adjustment_->property_value());
}

void SoundThemeEditor::updateThemeBindings()
{
  unbindProperties(); // seems to be necessary before rebinding properties

  if (auto settings = settings::soundThemeList()->settings(theme_id_); settings)
  {
    auto& key =
        strong_radio_button_->get_active() ? settings::kKeySoundThemeStrongParams
      : mid_radio_button_->get_active() ? settings::kKeySoundThemeMidParams
      : settings::kKeySoundThemeWeakParams;

    bindProperties(settings, key);
  }
}

void SoundThemeEditor::onSettingsListChanged(const Glib::ustring& key)
{
  if (key == settings::kKeySettingsListEntries)
  {
    if (!settings::soundThemeList()->contains(theme_id_))
      {
        unbindProperties();

        main_box_->set_sensitive(false);
        parameters_frame_->set_visible(false);
        unavailable_label_->set_visible(true);
      }
    else if (unavailable_label_->is_visible())
    {
      // the sound theme is available again

      unavailable_label_->set_visible(false);
      parameters_frame_->set_visible(true);
      main_box_->set_sensitive(true);

      updateThemeBindings();
    }
  }
  else if (key == settings::kKeySettingsListSelectedEntry)
  {
    // nothing to do
  }
}
