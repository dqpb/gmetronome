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

#ifndef GMetronome_SoundThemeEditor_h
#define GMetronome_SoundThemeEditor_h

#include "Settings.h"
#include "AccentButton.h"
#include <gtkmm.h>

class ShapeButton : public Gtk::Button
{
public:
  enum class Mode { kAttack, kHold, kDecay };

public:
  ShapeButton(Mode mode = Mode::kAttack);

  virtual ~ShapeButton();

  Glib::PropertyProxy<Glib::ustring> property_shape()
    { return property_shape_.get_proxy(); }

private:
  Glib::Property<Glib::ustring> property_shape_;
  Mode mode_;

  void next(bool cycle = true);
  void prev(bool cycle = true);

  void on_clicked() override;
  bool on_scroll_event(GdkEventScroll *scroll_event) override;
  void onShapeChanged();
};

/**
 * Sound Theme Editor dialog
 */
class SoundThemeEditor : public Gtk::Window
{
public:
  SoundThemeEditor(BaseObjectType* obj,
                   const Glib::RefPtr<Gtk::Builder>& builder,
                   Glib::ustring theme_id);

  ~SoundThemeEditor();

  static SoundThemeEditor* create(Gtk::Window& parent, Glib::ustring theme_id);

private:
  Glib::RefPtr<Gtk::Builder> builder_;
  Glib::ustring theme_id_;

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
  Gtk::Box* percussion_attack_box_;
  Gtk::Box* percussion_hold_box_;
  Gtk::Box* percussion_decay_box_;
  ShapeButton tone_attack_shape_button_;
  ShapeButton tone_hold_shape_button_;
  ShapeButton tone_decay_shape_button_;
  ShapeButton percussion_attack_shape_button_;
  ShapeButton percussion_hold_shape_button_;
  ShapeButton percussion_decay_shape_button_;
  Gtk::Scale* pan_scale_;
  Gtk::Scale* volume_scale_;
  Glib::RefPtr<Gtk::Adjustment> tone_pitch_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_timbre_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_detune_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_attack_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_hold_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> tone_decay_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> percussion_cutoff_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> percussion_attack_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> percussion_hold_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> percussion_decay_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> mix_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> pan_adjustment_;
  Glib::RefPtr<Gtk::Adjustment> volume_adjustment_;

  Gtk::Label* unavailable_label_;

  AccentButtonDrawingArea strong_accent_drawing_;
  AccentButtonDrawingArea mid_accent_drawing_;
  AccentButtonDrawingArea weak_accent_drawing_;

  Glib::ustring title_new_;
  Glib::ustring title_duplicate_;
  Glib::ustring title_placeholder_;

private:
  Glib::RefPtr<Gio::Settings> sound_settings_;

  bool onKeyPressEvent(GdkEventKey* event);
  void unbindSoundProperties();
  void bindSoundProperties();
  void updateThemeBindings();
  void onSettingsListChanged(const Glib::ustring& key);

  // drag and drop handler
  void onParamsDragBegin (const Glib::RefPtr<Gdk::DragContext>&);
  void onParamsDragDataGet (Gtk::RadioButton*,
                            const Glib::RefPtr<Gdk::DragContext>&,
                            Gtk::SelectionData&);
  void onParamsDragDataReceived (Gtk::RadioButton*,
                                 const Glib::RefPtr<Gdk::DragContext>&,
                                 const Gtk::SelectionData&, guint);
};

#endif//GMetronome_SoundThemeEditor_h
