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

#ifndef GMetronome_SoundThemeEditor_h
#define GMetronome_SoundThemeEditor_h

#include "Settings.h"
#include "AccentButton.h"
#include "SoundThemeManager.h"
#include "Application.h"
#include "StateButton.h"

#include <gtkmm.h>
#include <string>

/**
 * Sound Theme Editor
 */
class SoundThemeEditor : public Gtk::Window {
public:
  //Type aliases
  using RampShapeButton = StateButton<audio::EnvelopeRampShape>;
  using HoldShapeButton = StateButton<audio::EnvelopeHoldShape>;

public:
  SoundThemeEditor(BaseObjectType* obj,
                   const Glib::RefPtr<Gtk::Builder>& builder,
                   SoundThemeManager::Identifier theme_id);

  ~SoundThemeEditor();

  static SoundThemeEditor* create(Gtk::Window& parent, SoundThemeManager::Identifier theme_id);

private:
  Glib::RefPtr<Gtk::Builder> builder_;
  Glib::RefPtr<Application> app_;
  SoundThemeManager& sound_theme_manager_;

  SoundThemeManager::Identifier theme_id_;
  audio::SoundParameters current_params_;

  Gtk::Box* main_box_;
  Gtk::Frame* parameters_frame_;
  Gtk::Entry* title_entry_;
  Gtk::RadioButton* strong_radio_button_;
  Gtk::RadioButton* mid_radio_button_;
  Gtk::RadioButton* weak_radio_button_;
  Gtk::Grid* parameters_grid_;
  Gtk::Box* tone_attack_box_;
  Gtk::Box* tone_hold_box_;
  Gtk::Box* tone_decay_box_;
  Gtk::Box* noise_attack_box_;
  Gtk::Box* noise_hold_box_;
  Gtk::Box* noise_decay_box_;

  RampShapeButton tone_attack_shape_button_;
  HoldShapeButton tone_hold_shape_button_;
  RampShapeButton tone_decay_shape_button_;
  RampShapeButton noise_attack_shape_button_;
  HoldShapeButton noise_hold_shape_button_;
  RampShapeButton noise_decay_shape_button_;

  Gtk::Scale* pan_scale_;
  Gtk::Scale* gain_scale_;
  Glib::RefPtr<Gtk::Adjustment> tone_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_detune_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_attack_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_hold_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_decay_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> noise_cutoff_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> noise_attack_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> noise_hold_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> noise_decay_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> mix_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> pan_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> gain_adjustment_;

  Gtk::Label* unavailable_label_;

  AccentButtonDrawingArea strong_accent_drawing_;
  AccentButtonDrawingArea mid_accent_drawing_;
  AccentButtonDrawingArea weak_accent_drawing_;

  sigc::connection title_connection_;
  std::vector<sigc::connection> parameter_connections_;

  Glib::ustring title_placeholder_;

private:
  bool onKeyPressEvent(GdkEventKey* event);

  void onTitleChanged();
  void onParametersChanged();

  void loadTitle(const std::string& title);
  void loadSoundTheme();
  void loadParameters(const audio::SoundParameters& params);

  void setAvailableMode(bool available = true);
  bool isAvailableMode() const;

  // Sound Theme Manager signal handlers
  void updateSoundThemeSelected(const SoundThemeManager::Identifier& id);
  void updateSoundThemeCreated(const SoundThemeManager::Identifier& id);
  void updateSoundThemeRemoved(const SoundThemeManager::Identifier& id);
  void updateSoundThemeUpdated(const SoundThemeManager::Identifier& id,
                               const SoundThemeManager::Patch& patch);
  // Drag and drop handler
  void onParamsDragBegin (const Glib::RefPtr<Gdk::DragContext>&);
  void onParamsDragDataGet (Gtk::RadioButton*,
                            const Glib::RefPtr<Gdk::DragContext>&,
                            Gtk::SelectionData&);
  void onParamsDragDataReceived (Gtk::RadioButton*,
                                 const Glib::RefPtr<Gdk::DragContext>&,
                                 const Gtk::SelectionData&, guint);
  // Helper
  template<typename P>
  void connectParameter(const Glib::RefPtr<Gtk::Adjustment>& adj, P& param) {
    parameter_connections_.push_back(
      adj->signal_value_changed().connect(
        [this, &adj, &param] () {
          param = adj->get_value();
          onParametersChanged();
        })
      );
  }
  template<>
  void connectParameter(const Glib::RefPtr<Gtk::Adjustment>& adj, audio::Decibel& param) {
    parameter_connections_.push_back(
      adj->signal_value_changed().connect(
        [this, &adj, &param] () {
          param = audio::Decibel(adj->get_value());
          onParametersChanged();
        })
      );
  }
  template<typename S>
  void connectParameter(StateButton<S>& button, S& param) {
    parameter_connections_.push_back(
      button.signalStateChanged().connect(
        [this, &param] (const auto& state) {
          param = state;
          onParametersChanged();
        })
      );
  }
};

#endif//GMetronome_SoundThemeEditor_h
