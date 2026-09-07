/*
 * Copyright (C) 2022-2026 The GMetronome Team
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
#include "Convert.h"

#include <glibmm/i18n.h>

#include <cassert>
#include <vector>
#include <string>
#include <functional>

#ifndef NDEBUG
# include <iostream>
#endif

namespace {
  static const SoundThemeEditor::RampShapeButton::Configuration kAttackShapeButtonConfig =
  {
    {audio::EnvelopeRampShape::kCubic,        "gm-curve-cubic-up-symbolic"},
    {audio::EnvelopeRampShape::kLinear,       "gm-curve-linear-up-symbolic"},
    {audio::EnvelopeRampShape::kCubicFlipped, "gm-curve-cubic-up-flipped-symbolic"}
  };
  static const SoundThemeEditor::HoldShapeButton::Configuration kHoldShapeButtonConfig =
  {
    {audio::EnvelopeHoldShape::kQuartic,      "gm-curve-quartic-symbolic"},
    {audio::EnvelopeHoldShape::kKeep,         "gm-curve-keep-symbolic"}
  };
  static const SoundThemeEditor::RampShapeButton::Configuration kDecayShapeButtonConfig =
  {
    {audio::EnvelopeRampShape::kCubic,        "gm-curve-cubic-down-symbolic"},
    {audio::EnvelopeRampShape::kLinear,       "gm-curve-linear-down-symbolic"},
    {audio::EnvelopeRampShape::kCubicFlipped, "gm-curve-cubic-down-flipped-symbolic"}
  };
}

SoundThemeEditor::SoundThemeEditor(BaseObjectType* obj,
                                   const Glib::RefPtr<Gtk::Builder>& builder,
                                   SoundThemeManager::Identifier theme_id)
  : Glib::ObjectBase("SoundThemeEditor"),
    Gtk::Window(obj),
    builder_(builder),
    app_{Glib::RefPtr<Application>::cast_dynamic(Gtk::Application::get_default())},
    sound_theme_manager_{app_->soundThemeManager()},
    theme_id_{std::move(theme_id)},
    tone_attack_shape_button_{kAttackShapeButtonConfig},
    tone_hold_shape_button_{kHoldShapeButtonConfig},
    tone_decay_shape_button_{kDecayShapeButtonConfig},
    noise_attack_shape_button_{kAttackShapeButtonConfig},
    noise_hold_shape_button_{kHoldShapeButtonConfig},
    noise_decay_shape_button_{kDecayShapeButtonConfig}
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
  builder_->get_widget("noiseAttackBox", noise_attack_box_);
  builder_->get_widget("noiseHoldBox", noise_hold_box_);
  builder_->get_widget("noiseDecayBox", noise_decay_box_);
  builder_->get_widget("panScale", pan_scale_);
  builder_->get_widget("volumeScale", volume_scale_);
  builder_->get_widget("unavailableLabel", unavailable_label_);

  title_placeholder_ =
    g_dpgettext2(NULL, "Sound theme", SoundTheme::kDefaultTitlePlaceholder.c_str());

  title_entry_->set_placeholder_text(title_placeholder_);

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
  noise_cutoff_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("noiseCutoffAdjustment"));
  noise_attack_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("noiseAttackAdjustment"));
  noise_hold_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("noiseHoldAdjustment"));
  noise_decay_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("noiseDecayAdjustment"));
  mix_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("mixAdjustment"));
  pan_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("panAdjustment"));
  volume_adjustment_ =
    Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder_->get_object("volumeAdjustment"));

  tone_attack_box_->pack_start(tone_attack_shape_button_, Gtk::PACK_SHRINK);
  tone_hold_box_->pack_start(tone_hold_shape_button_, Gtk::PACK_SHRINK);
  tone_decay_box_->pack_start(tone_decay_shape_button_, Gtk::PACK_SHRINK);
  noise_attack_box_->pack_start(noise_attack_shape_button_, Gtk::PACK_SHRINK);
  noise_hold_box_->pack_start(noise_hold_shape_button_, Gtk::PACK_SHRINK);
  noise_decay_box_->pack_start(noise_decay_shape_button_, Gtk::PACK_SHRINK);

  tone_attack_shape_button_.show();
  tone_hold_shape_button_.show();
  tone_decay_shape_button_.show();
  noise_attack_shape_button_.show();
  noise_hold_shape_button_.show();
  noise_decay_shape_button_.show();

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

  loadSoundTheme();

  signal_key_press_event().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::onKeyPressEvent));

  title_connection_ = title_entry_->signal_changed().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::onTitleChanged));

  strong_radio_button_->signal_clicked().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::loadSoundTheme));
  mid_radio_button_->signal_clicked().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::loadSoundTheme));
  weak_radio_button_->signal_clicked().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::loadSoundTheme));

  sound_theme_manager_.signalSelected().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::updateSoundThemeSelected));
  sound_theme_manager_.signalCreated().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::updateSoundThemeCreated));
  sound_theme_manager_.signalRemoved().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::updateSoundThemeRemoved));
  sound_theme_manager_.signalUpdated().connect(
    sigc::mem_fun(*this, &SoundThemeEditor::updateSoundThemeUpdated));

  // Tone oscillator parameters
  connectParameter( tone_pitch_adjustment_,    current_params_.tone_pitch );
  connectParameter( tone_timbre_adjustment_,   current_params_.tone_timbre );
  connectParameter( tone_detune_adjustment_,   current_params_.tone_detune );
  connectParameter( tone_attack_adjustment_,   current_params_.tone_attack );
  connectParameter( tone_attack_shape_button_, current_params_.tone_attack_shape );
  connectParameter( tone_hold_adjustment_,     current_params_.tone_hold );
  connectParameter( tone_hold_shape_button_,   current_params_.tone_hold_shape );
  connectParameter( tone_decay_adjustment_,    current_params_.tone_decay );
  connectParameter( tone_decay_shape_button_,  current_params_.tone_decay_shape );

  // Noise oscillator parameters
  connectParameter( noise_cutoff_adjustment_,   current_params_.noise_cutoff );
  connectParameter( noise_attack_adjustment_,   current_params_.noise_attack );
  connectParameter( noise_attack_shape_button_, current_params_.noise_attack_shape );
  connectParameter( noise_hold_adjustment_,     current_params_.noise_hold );
  connectParameter( noise_hold_shape_button_,   current_params_.noise_hold_shape );
  connectParameter( noise_decay_adjustment_,    current_params_.noise_decay );
  connectParameter( noise_decay_shape_button_,  current_params_.noise_decay_shape );

  // Mix, pan, volume
  connectParameter( mix_adjustment_,    current_params_.mix );
  connectParameter( pan_adjustment_,    current_params_.pan );
  connectParameter( volume_adjustment_, current_params_.volume );

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
  // nothing
}

//static
SoundThemeEditor* SoundThemeEditor::create(Gtk::Window& parent,
                                           SoundThemeManager::Identifier theme_id)
{
  // load the Builder file and instantiate the dialog
  auto resource_path = Glib::ustring(PACKAGE_ID_PATH) + "/ui/SoundThemeEditor.ui";
  auto builder = Gtk::Builder::create_from_resource(resource_path);

  SoundThemeEditor* dialog = nullptr;
  builder->get_widget_derived("editorWindow", dialog, std::move(theme_id));
  if (!dialog)
    throw std::runtime_error("No \"editorWindow\" object in SoundThemeEditor.ui");

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

void SoundThemeEditor::onTitleChanged()
{
  SoundThemeManager::Patch patch;
  patch.title = title_entry_->get_text();
  sound_theme_manager_.update(theme_id_, patch);
}

void SoundThemeEditor::onParametersChanged()
{
  SoundThemeManager::Patch patch;

  if (strong_radio_button_->get_active())
    patch.strong_params = current_params_;
  else if (mid_radio_button_->get_active())
    patch.mid_params = current_params_;
  else if (weak_radio_button_->get_active())
    patch.weak_params = current_params_;

  sound_theme_manager_.update(theme_id_, patch);
}

void SoundThemeEditor::loadTitle(const std::string& title)
{
  title_connection_.block();
  title_entry_->set_text(title);
  title_connection_.unblock();
}

void SoundThemeEditor:: loadSoundTheme()
{
  if (auto theme = sound_theme_manager_.get(theme_id_))
  {
    loadTitle(theme->header.title);

    if (strong_radio_button_->get_active())
      loadParameters(theme->content.strong_params);
    else if (mid_radio_button_->get_active())
      loadParameters(theme->content.mid_params);
    else if (weak_radio_button_->get_active())
      loadParameters(theme->content.weak_params);

    if (!isAvailableMode())
      setAvailableMode(true);
  }
  else if (isAvailableMode()) {
    setAvailableMode(false);
  }
}

void SoundThemeEditor:: loadParameters(const audio::SoundParameters& params)
{
  for (auto& connection : parameter_connections_)
    connection.block();

  tone_pitch_adjustment_->set_value(params.tone_pitch);
  tone_timbre_adjustment_->set_value(params.tone_timbre);
  tone_detune_adjustment_->set_value(params.tone_detune);
  tone_attack_adjustment_->set_value(params.tone_attack);
  tone_attack_shape_button_.setState(params.tone_attack_shape);
  tone_hold_adjustment_->set_value(params.tone_hold);
  tone_hold_shape_button_.setState(params.tone_hold_shape);
  tone_decay_adjustment_->set_value(params.tone_decay);
  tone_decay_shape_button_.setState(params.tone_decay_shape);

  noise_cutoff_adjustment_->set_value(params.noise_cutoff);
  noise_attack_adjustment_->set_value(params.noise_attack);
  noise_attack_shape_button_.setState(params.noise_attack_shape);
  noise_hold_adjustment_->set_value(params.noise_hold);
  noise_hold_shape_button_.setState(params.noise_hold_shape);
  noise_decay_adjustment_->set_value(params.noise_decay);
  noise_decay_shape_button_.setState(params.noise_decay_shape);

  mix_adjustment_->set_value(params.mix);
  pan_adjustment_->set_value(params.pan);
  volume_adjustment_->set_value(params.volume);

  current_params_ = params;

  for (auto& connection : parameter_connections_)
    connection.unblock();
}

void SoundThemeEditor::setAvailableMode(bool available)
{
  if (available && !isAvailableMode())
  {
    // sound theme is available again
    unavailable_label_->set_visible(false);
    parameters_frame_->set_visible(true);
    main_box_->set_sensitive(true);
  }
  else if (!available && isAvailableMode())
  {
    main_box_->set_sensitive(false);
    parameters_frame_->set_visible(false);
    unavailable_label_->set_visible(true);
  }
}

bool SoundThemeEditor::isAvailableMode() const
{
  return !unavailable_label_->get_visible();
}

void SoundThemeEditor::updateSoundThemeSelected(const SoundThemeManager::Identifier& id)
{
  // nothing
}

void SoundThemeEditor::updateSoundThemeCreated(const SoundThemeManager::Identifier& id)
{
  if (id == theme_id_)
    loadSoundTheme();
}

void SoundThemeEditor::updateSoundThemeRemoved(const SoundThemeManager::Identifier& id)
{
  if (id == theme_id_)
    setAvailableMode(false);
}

void SoundThemeEditor::updateSoundThemeUpdated(const SoundThemeManager::Identifier& id,
                                               const SoundThemeManager::Patch& patch)
{
  if (id != theme_id_)
    return;

  if (patch.title)
    loadTitle(*patch.title);

  if (patch.strong_params && strong_radio_button_->get_active())
    loadParameters(*patch.strong_params);
  else if (patch.mid_params && mid_radio_button_->get_active())
    loadParameters(*patch.mid_params);
  else if (patch.weak_params && weak_radio_button_->get_active())
    loadParameters(*patch.weak_params);
}

// Drag and drop
namespace {
  struct ParamKeyReference
  {
    const std::string key;
    std::variant<
      std::reference_wrapper<float>,
      std::reference_wrapper<audio::EnvelopeRampShape>,
      std::reference_wrapper<audio::EnvelopeHoldShape>
      > value;
  };

  std::vector<ParamKeyReference> paramKeyReferenceMap(audio::SoundParameters& params)
  {
    return {
      {"tone-pitch",         std::ref(params.tone_pitch) },
      {"tone-timbre",        std::ref(params.tone_timbre)},
      {"tone-detune",        std::ref(params.tone_detune)},
      {"tone-attack",        std::ref(params.tone_attack)},
      {"tone-attack-shape",  std::ref(params.tone_attack_shape)},
      {"tone-hold",          std::ref(params.tone_hold)},
      {"tone-hold-shape",    std::ref(params.tone_hold_shape)},
      {"tone-decay",         std::ref(params.tone_decay)},
      {"tone-decay-shape",   std::ref(params.tone_decay_shape)},

      {"noise-cutoff",       std::ref(params.noise_cutoff)},
      {"noise-attack",       std::ref(params.noise_attack)},
      {"noise-attack-shape", std::ref(params.noise_attack_shape)},
      {"noise-hold",         std::ref(params.noise_hold)},
      {"noise-hold-shape",   std::ref(params.noise_hold_shape)},
      {"noise-decay",        std::ref(params.noise_decay)},
      {"noise-decay-shape",  std::ref(params.noise_decay_shape)},

      {"mix",      std::ref(params.mix)},
      {"pan",      std::ref(params.pan)},
      {"volume",   std::ref(params.volume)}
    };
  }
}//unnamed namespace

void SoundThemeEditor::onParamsDragBegin(const Glib::RefPtr<Gdk::DragContext>& context)
{
  // nothing
}

void SoundThemeEditor::onParamsDragDataGet(Gtk::RadioButton* source_button,
                                           const Glib::RefPtr<Gdk::DragContext>& context,
                                           Gtk::SelectionData& data)
{
  if (auto theme = sound_theme_manager_.get(theme_id_))
  {
    audio::SoundParameters* params = nullptr;
    Glib::ustring params_group;

    if (source_button == strong_radio_button_) {
      params = &theme->content.strong_params;
      params_group = settings::kSchemaPathSoundThemeStrongParamsBasename;
    }
    else if (source_button == mid_radio_button_) {
      params = &theme->content.mid_params;
      params_group = settings::kSchemaPathSoundThemeMidParamsBasename;
    }
    else if (source_button == weak_radio_button_) {
      params = &theme->content.weak_params;
      params_group = settings::kSchemaPathSoundThemeWeakParamsBasename;
    }

    Glib::KeyFile key_file;

    for (auto& key_value : paramKeyReferenceMap(*params))
    {
      const auto& key = key_value.key;
      auto& value = key_value.value;

      std::visit( [&] (auto& ref) {
      using T = std::decay_t<decltype(ref)>;
      if constexpr (std::is_same_v<T, std::reference_wrapper<float>>)
        key_file.set_double(params_group, key, ref.get());
      if constexpr (std::is_same_v<T, std::reference_wrapper<audio::EnvelopeRampShape>>)
        key_file.set_string(params_group, key, rampShapeToString(ref.get()));
      if constexpr (std::is_same_v<T, std::reference_wrapper<audio::EnvelopeHoldShape>>)
        key_file.set_string(params_group, key, holdShapeToString(ref.get()));
      }, value);
    }

    data.set(data.get_target(), key_file.to_data());
  }
}

void SoundThemeEditor::onParamsDragDataReceived(Gtk::RadioButton* target_button,
                                                const Glib::RefPtr<Gdk::DragContext>& context,
                                                const Gtk::SelectionData& data,
                                                guint time)
{
  auto theme = sound_theme_manager_.get(theme_id_);

  if (!theme) {
    context->drag_finish(false, false, time);
    return;
  }

  SoundThemeManager::Patch patch;
  audio::SoundParameters* params = nullptr;
  Glib::ustring params_group;

  if (target_button == strong_radio_button_) {
    patch.strong_params = theme->content.strong_params;
    params = &*patch.strong_params;
    params_group = settings::kSchemaPathSoundThemeStrongParamsBasename;
  }
  else if (target_button == mid_radio_button_) {
    patch.mid_params = theme->content.mid_params;
    params = &*patch.mid_params;
    params_group = settings::kSchemaPathSoundThemeMidParamsBasename;
  }
  else if (target_button == weak_radio_button_) {
    patch.weak_params = theme->content.weak_params;
    params = &*patch.weak_params;
    params_group = settings::kSchemaPathSoundThemeWeakParamsBasename;
  }

  try {
    Glib::KeyFile key_file;
    key_file.load_from_data(data.get_data_as_string());

    if (!key_file.has_group(params_group))
      params_group = key_file.get_start_group();

    for (auto& key_value : paramKeyReferenceMap(*params))
    {
      const auto& key = key_value.key;
      auto& value = key_value.value;

      if (key_file.has_key(params_group, key))
      {
        std::visit( [&] (auto& ref) {
          using T = std::decay_t<decltype(ref)>;
          if constexpr (std::is_same_v<T, std::reference_wrapper<float>>)
            ref.get() = key_file.get_double(params_group, key);
          if constexpr (std::is_same_v<T, std::reference_wrapper<audio::EnvelopeRampShape>>)
            ref.get() = stringToRampShape(key_file.get_string(params_group, key));
          if constexpr (std::is_same_v<T, std::reference_wrapper<audio::EnvelopeHoldShape>>)
            ref.get() = stringToHoldShape(key_file.get_string(params_group, key));
        }, value);
      }
    }
  }
  catch(const Glib::Exception& e)
  {
#ifndef NDEBUG
    std::cerr << "SoundThemeEditor: Invalid drop data. (" <<  e.what() << ")" << std::endl;
#endif
    context->drag_finish(false, false, time);
    return;
  }
  catch (...)
  {
#ifndef NDEBUG
    std::cerr << "SoundThemeEditor: Invalid drop data." << std::endl;
#endif
    context->drag_finish(false, false, time);
    return;
  }

  if (sound_theme_manager_.update(theme_id_, patch))
    context->drag_finish(true, false, time);
  else
    context->drag_finish(false, false, time);
}
