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
  builder_->get_widget("percussiveClapSwitch", percussive_clap_switch_);
  builder_->get_widget("bellSwitch", bell_switch_);
  builder_->get_widget("balanceScale", balance_scale_);
  builder_->get_widget("unavailableLabel", unavailable_label_);

  tonal_pitch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonalPitchAdjustment"));
  tonal_timbre_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonalTimbreAdjustment"));
  tonal_detune_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonalDetuneAdjustment"));
  tonal_punch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonalPunchAdjustment"));
  tonal_decay_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonalDecayAdjustment"));
  percussive_tone_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussiveToneAdjustment"));
  percussive_punch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussivePunchAdjustment"));
  percussive_decay_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussiveDecayAdjustment"));
  mix_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("mixAdjustment"));
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

  settings::soundThemes()->settings()->signal_changed()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onSettingsListChanged));

  updateThemeBindings();
}

SoundThemeEditor::~SoundThemeEditor()
{ /* nothing */ }

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

void SoundThemeEditor::bindSoundProperties()
{
  if (sound_settings_)
  {
    sound_settings_->bind(settings::kKeySoundThemeTonalPitch,
                          tonal_pitch_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeTonalTimbre,
                          tonal_timbre_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeTonalDetune,
                          tonal_detune_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeTonalPunch,
                          tonal_punch_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeTonalDecay,
                          tonal_decay_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussiveTone,
                          percussive_tone_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussivePunch,
                          percussive_punch_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussiveDecay,
                          percussive_decay_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeMix,
                          mix_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeBalance,
                          balance_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeVolume,
                          volume_adjustment_->property_value());
    //...
  }
}

// helper
void unbindProperty(const Glib::PropertyProxy_Base& p)
{
  g_settings_unbind(p.get_object()->gobj(), p.get_name());
}

void SoundThemeEditor::unbindSoundProperties()
{
  unbindProperty(tonal_pitch_adjustment_->property_value());
  unbindProperty(tonal_timbre_adjustment_->property_value());
  unbindProperty(tonal_detune_adjustment_->property_value());
  unbindProperty(tonal_punch_adjustment_->property_value());
  unbindProperty(tonal_decay_adjustment_->property_value());
  unbindProperty(percussive_tone_adjustment_->property_value());
  unbindProperty(percussive_punch_adjustment_->property_value());
  unbindProperty(percussive_decay_adjustment_->property_value());
  unbindProperty(mix_adjustment_->property_value());
  unbindProperty(balance_adjustment_->property_value());
  unbindProperty(volume_adjustment_->property_value());
  //...
}

void SoundThemeEditor::updateThemeBindings()
{
  unbindSoundProperties();
  unbindProperty(title_entry_->property_text());

  if (auto& settings_tree = settings::soundThemes()->settings(theme_id_);
      settings_tree.settings)
  {
    auto& children = settings_tree.children;

    if (strong_radio_button_->get_active())
      sound_settings_ = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
    else if (mid_radio_button_->get_active())
      sound_settings_ = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
    else
      sound_settings_ = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

    settings_tree.settings->bind(settings::kKeySoundThemeTitle, title_entry_->property_text());
    bindSoundProperties();
  }
}

void SoundThemeEditor::onSettingsListChanged(const Glib::ustring& key)
{
  if (key == settings::kKeySettingsListEntries)
  {
    if (!settings::soundThemes()->contains(theme_id_))
      {
        main_box_->set_sensitive(false);
        parameters_frame_->set_visible(false);
        unavailable_label_->set_visible(true);
      }
    else if (unavailable_label_->is_visible())
    {
      // sound theme is available again
      unavailable_label_->set_visible(false);
      parameters_frame_->set_visible(true);
      main_box_->set_sensitive(true);

      updateThemeBindings();
    }
  }
  else if (key == settings::kKeySettingsListSelectedEntry)
  {
    /* nothing */
  }
}
