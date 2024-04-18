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
#include <vector>
#include <string>

#ifndef NDEBUG
# include <iostream>
#endif

ShapeButton::ShapeButton(Mode mode)
  : Glib::ObjectBase("ShapeButton"),
    property_shape_(*this, "shape"),
    mode_{mode}
{
  switch (mode_) {
  case Mode::kAttack:
    property_shape_ = "linear";
    set_image_from_icon_name("gm-curve-linear-up-symbolic");
    break;
  case Mode::kHold:
    property_shape_ = "keep";
    set_image_from_icon_name("gm-curve-keep-symbolic");
    break;
  case Mode::kDecay:
    property_shape_ = "linear";
    set_image_from_icon_name("gm-curve-linear-down-symbolic");
    break;
  default:
    break;
  };

  property_shape_.get_proxy().signal_changed()
    .connect(sigc::mem_fun(*this, &ShapeButton::onShapeChanged));

  add_events(Gdk::SCROLL_MASK);
}

ShapeButton::~ShapeButton()
{ /* nothing */ }

void ShapeButton::next(bool cycle)
{
  auto current_shape = property_shape_.get_value();
  Glib::ustring next_shape = current_shape;

  if (mode_ == Mode::kHold) {
    if (current_shape == "quartic")
      next_shape = "keep";
    else if (current_shape == "keep" && cycle)
      next_shape = "quartic";
  }
  else {
    if (current_shape == "cubic")
      next_shape = "linear";
    else if (current_shape == "linear")
      next_shape = "cubic-flipped";
    else if (current_shape == "cubic-flipped" && cycle)
      next_shape = "cubic";
  }
  property_shape_.set_value( next_shape );
}

void ShapeButton::prev(bool cycle)
{
  auto current_shape = property_shape_.get_value();
  Glib::ustring prev_shape = current_shape;

  if (mode_ == Mode::kHold) {
    if (current_shape == "keep")
      prev_shape = "quartic";
    else if (current_shape == "quartic" && cycle)
      prev_shape = "keep";
  }
  else {
    if (current_shape == "cubic-flipped")
      prev_shape = "linear";
    else if (current_shape == "linear")
      prev_shape = "cubic";
    else if (current_shape == "cubic" && cycle)
      prev_shape = "cubic-flipped";
  }
  property_shape_.set_value( prev_shape );
}

void ShapeButton::on_clicked()
{
  next(true);
  Button::on_clicked();
}

bool ShapeButton::on_scroll_event(GdkEventScroll *scroll_event)
{
  switch (scroll_event->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_RIGHT:
    next(false);
    break;

  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_LEFT:
    prev(false);
    break;

  default:
    break;
  };

  return Gtk::Button::on_scroll_event(scroll_event);
}

void ShapeButton::onShapeChanged()
{
  auto current_shape = property_shape_.get_value();

  if (mode_ == Mode::kAttack) {
    if (current_shape == "linear")
      set_image_from_icon_name("gm-curve-linear-up-symbolic");
    else if (current_shape == "cubic")
      set_image_from_icon_name("gm-curve-cubic-up-symbolic");
    else if (current_shape == "cubic-flipped")
      set_image_from_icon_name("gm-curve-cubic-up-flipped-symbolic");
    else
      set_image_from_icon_name("");
  }
  else if (mode_ == Mode::kHold)
  {
    if (current_shape == "keep")
      set_image_from_icon_name("gm-curve-keep-symbolic");
    else if (current_shape == "quartic")
      set_image_from_icon_name("gm-curve-quartic-symbolic");
    else
      set_image_from_icon_name("");
  }
  else if (mode_ == Mode::kDecay) {
    if (current_shape == "linear")
      set_image_from_icon_name("gm-curve-linear-down-symbolic");
    else if (current_shape == "cubic")
      set_image_from_icon_name("gm-curve-cubic-down-symbolic");
    else if (current_shape == "cubic-flipped")
      set_image_from_icon_name("gm-curve-cubic-down-flipped-symbolic");
    else
      set_image_from_icon_name("");
  }
}

SoundThemeEditor::SoundThemeEditor(BaseObjectType* obj,
                                   const Glib::RefPtr<Gtk::Builder>& builder,
                                   Glib::ustring theme_id)
  : Glib::ObjectBase("SoundThemeEditor"),
    Gtk::Window(obj),
    builder_(builder),
    theme_id_{std::move(theme_id)},
    tone_attack_shape_button_{ShapeButton::Mode::kAttack},
    tone_hold_shape_button_{ShapeButton::Mode::kHold},
    tone_decay_shape_button_{ShapeButton::Mode::kDecay},
    percussion_attack_shape_button_{ShapeButton::Mode::kAttack},
    percussion_hold_shape_button_{ShapeButton::Mode::kHold},
    percussion_decay_shape_button_{ShapeButton::Mode::kDecay}
{
  builder_->get_widget("mainBox", main_box_);
  builder_->get_widget("parametersFrame", parameters_frame_);
  builder_->get_widget("titleEntry", title_entry_);
  builder_->get_widget("strongRadioButton", strong_radio_button_);
  builder_->get_widget("midRadioButton", mid_radio_button_);
  builder_->get_widget("weakRadioButton", weak_radio_button_);
  builder_->get_widget("parametersGrid", parameters_grid_);
  builder_->get_widget("toneAttackBox", tone_attack_box_);
  builder_->get_widget("toneHoldBox", tone_hold_box_);
  builder_->get_widget("toneDecayBox", tone_decay_box_);
  builder_->get_widget("percussionAttackBox", percussion_attack_box_);
  builder_->get_widget("percussionHoldBox", percussion_hold_box_);
  builder_->get_widget("percussionDecayBox", percussion_decay_box_);
  builder_->get_widget("panScale", pan_scale_);
  builder_->get_widget("volumeScale", volume_scale_);
  builder_->get_widget("unavailableLabel", unavailable_label_);

  tone_pitch_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("tonePitchAdjustment"));
  tone_timbre_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("toneTimbreAdjustment"));
  tone_detune_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("toneDetuneAdjustment"));
  tone_attack_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("toneAttackAdjustment"));
  tone_hold_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("toneHoldAdjustment"));
  tone_decay_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("toneDecayAdjustment"));
  percussion_cutoff_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussionCutoffAdjustment"));
  percussion_attack_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussionAttackAdjustment"));
  percussion_hold_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussionHoldAdjustment"));
  percussion_decay_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("percussionDecayAdjustment"));
  mix_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("mixAdjustment"));
  pan_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("panAdjustment"));
  volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("volumeAdjustment"));

  tone_attack_box_->pack_start(tone_attack_shape_button_, Gtk::PACK_SHRINK);
  tone_hold_box_->pack_start(tone_hold_shape_button_, Gtk::PACK_SHRINK);
  tone_decay_box_->pack_start(tone_decay_shape_button_, Gtk::PACK_SHRINK);
  percussion_attack_box_->pack_start(percussion_attack_shape_button_, Gtk::PACK_SHRINK);
  percussion_hold_box_->pack_start(percussion_hold_shape_button_, Gtk::PACK_SHRINK);
  percussion_decay_box_->pack_start(percussion_decay_shape_button_, Gtk::PACK_SHRINK);

  tone_attack_shape_button_.show();
  tone_hold_shape_button_.show();
  tone_decay_shape_button_.show();
  percussion_attack_shape_button_.show();
  percussion_hold_shape_button_.show();
  percussion_decay_shape_button_.show();

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

  pan_scale_->add_mark(0.0, Gtk::POS_BOTTOM, "");
  volume_scale_->add_mark(100.0, Gtk::POS_BOTTOM, "");

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

  //sound parameters drag and drop
  std::vector<Gtk::TargetEntry> targets = {Gtk::TargetEntry{"text/plain"}};

  strong_radio_button_->drag_source_set(targets);
  mid_radio_button_->drag_source_set(targets);
  weak_radio_button_->drag_source_set(targets);

  strong_radio_button_->drag_dest_set(targets);
  mid_radio_button_->drag_dest_set(targets);
  weak_radio_button_->drag_dest_set(targets);

  // begin
  strong_radio_button_->signal_drag_begin()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onParamsDragBegin));
  mid_radio_button_->signal_drag_begin()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onParamsDragBegin));
  weak_radio_button_->signal_drag_begin()
    .connect(sigc::mem_fun(*this, &SoundThemeEditor::onParamsDragBegin));

  // get
  strong_radio_button_->signal_drag_data_get().connect(
    [&] (auto& context, auto& data, guint, guint) {
      onParamsDragDataGet(strong_radio_button_, context, data); });
  mid_radio_button_->signal_drag_data_get().connect(
    [&] (auto& context, auto& data, guint, guint) {
      onParamsDragDataGet(mid_radio_button_, context, data); });
  weak_radio_button_->signal_drag_data_get().connect(
    [&] (auto& context, auto& data, guint, guint) {
      onParamsDragDataGet(weak_radio_button_, context, data); });

  // received
  strong_radio_button_->signal_drag_data_received().connect(
    [&] (auto& context, int, int, auto& data, guint, guint time) {
      onParamsDragDataReceived(strong_radio_button_, context, data, time);
    });
  mid_radio_button_->signal_drag_data_received().connect(
    [&] (auto& context, int, int, auto& data, guint, guint time) {
      onParamsDragDataReceived(mid_radio_button_, context, data, time);
    });
  weak_radio_button_->signal_drag_data_received().connect(
    [&] (auto& context, int, int, auto& data, guint, guint time) {
      onParamsDragDataReceived(weak_radio_button_, context, data, time);
    });
}

SoundThemeEditor::~SoundThemeEditor()
{
  if (sound_settings_)
    sound_settings_->apply();
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

void SoundThemeEditor::bindSoundProperties()
{
  if (sound_settings_)
  {
    sound_settings_->bind(settings::kKeySoundThemeTonePitch,
                          tone_pitch_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneTimbre,
                          tone_timbre_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneDetune,
                          tone_detune_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneAttack,
                          tone_attack_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneAttackShape,
                          tone_attack_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemeToneHold,
                          tone_hold_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneHoldShape,
                          tone_hold_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemeToneDecay,
                          tone_decay_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeToneDecayShape,
                          tone_decay_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemePercussionCutoff,
                          percussion_cutoff_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussionAttack,
                          percussion_attack_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussionAttackShape,
                          percussion_attack_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemePercussionHold,
                          percussion_hold_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussionHoldShape,
                          percussion_hold_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemePercussionDecay,
                          percussion_decay_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePercussionDecayShape,
                          percussion_decay_shape_button_.property_shape());
    sound_settings_->bind(settings::kKeySoundThemeMix,
                          mix_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemePan,
                          pan_adjustment_->property_value());
    sound_settings_->bind(settings::kKeySoundThemeVolume,
                          volume_adjustment_->property_value());
  }
}

// helper
void unbindProperty(const Glib::PropertyProxy_Base& p)
{
  g_settings_unbind(p.get_object()->gobj(), p.get_name());
}

void SoundThemeEditor::unbindSoundProperties()
{
  unbindProperty(tone_pitch_adjustment_->property_value());
  unbindProperty(tone_timbre_adjustment_->property_value());
  unbindProperty(tone_detune_adjustment_->property_value());
  unbindProperty(tone_attack_adjustment_->property_value());
  unbindProperty(tone_attack_shape_button_.property_shape());
  unbindProperty(tone_hold_adjustment_->property_value());
  unbindProperty(tone_hold_shape_button_.property_shape());
  unbindProperty(tone_decay_adjustment_->property_value());
  unbindProperty(tone_decay_shape_button_.property_shape());
  unbindProperty(percussion_cutoff_adjustment_->property_value());
  unbindProperty(percussion_attack_adjustment_->property_value());
  unbindProperty(percussion_attack_shape_button_.property_shape());
  unbindProperty(percussion_hold_adjustment_->property_value());
  unbindProperty(percussion_hold_shape_button_.property_shape());
  unbindProperty(percussion_decay_adjustment_->property_value());
  unbindProperty(percussion_decay_shape_button_.property_shape());
  unbindProperty(mix_adjustment_->property_value());
  unbindProperty(pan_adjustment_->property_value());
  unbindProperty(volume_adjustment_->property_value());
}

void SoundThemeEditor::updateThemeBindings()
{
  unbindSoundProperties();
  unbindProperty(title_entry_->property_text());

  if (sound_settings_)
    sound_settings_->apply();

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

    if (sound_settings_)
      sound_settings_->delay();

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

namespace {
  struct ParamType_
  {
    const Glib::ustring& key;
    GType type;
  };

  const std::vector<ParamType_> params_type_map_  =
  {
    {settings::kKeySoundThemeTonePitch,       G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneTimbre,      G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneDetune,      G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneAttack,      G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneAttackShape, G_TYPE_ENUM},
    {settings::kKeySoundThemeToneHold,        G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneHoldShape,   G_TYPE_ENUM},
    {settings::kKeySoundThemeToneDecay,       G_TYPE_DOUBLE},
    {settings::kKeySoundThemeToneDecayShape,  G_TYPE_ENUM},

    {settings::kKeySoundThemePercussionCutoff,      G_TYPE_DOUBLE},
    {settings::kKeySoundThemePercussionAttack,      G_TYPE_DOUBLE},
    {settings::kKeySoundThemePercussionAttackShape, G_TYPE_ENUM},
    {settings::kKeySoundThemePercussionHold,        G_TYPE_DOUBLE},
    {settings::kKeySoundThemePercussionHoldShape,   G_TYPE_ENUM},
    {settings::kKeySoundThemePercussionDecay,       G_TYPE_DOUBLE},
    {settings::kKeySoundThemePercussionDecayShape,  G_TYPE_ENUM},

    {settings::kKeySoundThemeMix,    G_TYPE_DOUBLE},
    {settings::kKeySoundThemePan,    G_TYPE_DOUBLE},
    {settings::kKeySoundThemeVolume, G_TYPE_DOUBLE}
  };
}//unnamed namespace

void SoundThemeEditor::onParamsDragBegin(const Glib::RefPtr<Gdk::DragContext>& context)
{ /* nothing */ }

void SoundThemeEditor::onParamsDragDataGet(Gtk::RadioButton* source_button,
                                           const Glib::RefPtr<Gdk::DragContext>& context,
                                           Gtk::SelectionData& data)
{
  if (auto& settings_tree = settings::soundThemes()->settings(theme_id_);
      settings_tree.settings)
  {
    Glib::ustring params_group;

    if (source_button == strong_radio_button_)
      params_group = settings::kSchemaPathSoundThemeStrongParamsBasename;
    else if (source_button == mid_radio_button_)
      params_group = settings::kSchemaPathSoundThemeMidParamsBasename;
    else if (source_button == weak_radio_button_)
      params_group = settings::kSchemaPathSoundThemeWeakParamsBasename;

    if (auto settings = settings_tree.children.at(params_group).settings; settings)
    {
      Glib::KeyFile keys;

      for (auto& entry : params_type_map_)
      {
        if (entry.type == G_TYPE_DOUBLE)
          keys.set_double(params_group, entry.key, settings->get_double(entry.key));
        else if (entry.type == G_TYPE_BOOLEAN)
          keys.set_boolean(params_group, entry.key, settings->get_boolean(entry.key));
        else if (entry.type == G_TYPE_ENUM)
          keys.set_string(params_group, entry.key, settings->get_string(entry.key));
      }

      data.set(data.get_target(), keys.to_data());
    }
  }
}

void SoundThemeEditor::onParamsDragDataReceived(Gtk::RadioButton* target_button,
                                                const Glib::RefPtr<Gdk::DragContext>& context,
                                                const Gtk::SelectionData& data,
                                                guint time)
{
  if (auto& settings_tree = settings::soundThemes()->settings(theme_id_);
      settings_tree.settings)
  {
    Glib::ustring params_group;

    if (target_button == strong_radio_button_)
      params_group = settings::kSchemaPathSoundThemeStrongParamsBasename;
    else if (target_button == mid_radio_button_)
      params_group = settings::kSchemaPathSoundThemeMidParamsBasename;
    else if (target_button == weak_radio_button_)
      params_group = settings::kSchemaPathSoundThemeWeakParamsBasename;

    if (auto settings = settings_tree.children.at(params_group).settings; settings)
    {
      try {
        Glib::KeyFile keys;
        keys.load_from_data(data.get_data_as_string());

        if (!keys.has_group(params_group))
          params_group = keys.get_start_group();

        for (auto& entry : params_type_map_)
        {
          if (keys.has_key(params_group, entry.key))
          {
            if (entry.type == G_TYPE_DOUBLE)
              settings->set_double(entry.key, keys.get_double(params_group, entry.key));
            else if (entry.type == G_TYPE_BOOLEAN)
              settings->set_boolean(entry.key, keys.get_boolean(params_group, entry.key));
            else if (entry.type == G_TYPE_ENUM)
              settings->set_string(entry.key, keys.get_string(params_group, entry.key));
          }
        }
      }
      catch(const Glib::Exception& e)
      {
#ifndef NDEBUG
        std::cerr << "SoundThemeEditor: invalid drop data (" <<  e.what() << ")" << std::endl;
#endif
        context->drag_finish(false, false, time);
      }
      context->drag_finish(true, false, time);
    }
  }
  context->drag_finish(false, false, time);
}
