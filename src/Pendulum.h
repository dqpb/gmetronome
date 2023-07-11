/*
 * Copyright (C) 2021-2023 The GMetronome Team
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

#ifndef GMetronome_Pendulum_h
#define GMetronome_Pendulum_h

#include <gtkmm.h>
#include <array>
#include <chrono>
#include "Ticker.h"
#include "Physics.h"

class Pendulum : public Gtk::Widget {
public:
  enum class ActionAngle
  {
    kCenter,
    kReal,
    kEdge
  };

  enum class PhaseMode
  {
    kLeft  = 0,
    kRight = 1
  };

public:
  Pendulum();
  ~Pendulum() override = default;

  void start();
  void stop();

  void synchronize(const audio::Ticker::Statistics& stats,
                   const std::chrono::microseconds& sync);

  void setAction(ActionAngle angle);

  void setPhaseMode(PhaseMode mode);

  void togglePhase();

private:
  physics::PendulumKinematics k_;
  double action_angle_{0.0};
  PhaseMode phase_mode_{PhaseMode::kLeft};
  double phase_mode_shift_{0.0};
  double target_omega_{0.0};
  double beat_pos_{0.0};
  std::chrono::microseconds last_frame_time_{0};
  double needle_amplitude_{0.0};
  double needle_theta_{0.0};
  double needle_length_{0.9};
  std::array<double,2> needle_base_{0.5, 1.0};
  std::array<double,2> needle_tip_{0.5, 0.0};
  double dial_radius_{1.0};
  double dial_amplitude_{0.0};

  enum State {
    kShutdown   = 0,
    kStop       = 1,
    kStartup    = 2,
    kFillBuffer = 3,
    kRegular    = 4
  };

  State state_{kStop};

  void startAnimation();
  bool updateAnimation(const Glib::RefPtr<Gdk::FrameClock>&);

  Gdk::RGBA getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;
  Gdk::RGBA getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;

  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

  void draw_dial(const Cairo::RefPtr<Cairo::Context>& cr, Gdk::RGBA color);
  void draw_needle(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& color);
  void draw_knob(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::RGBA& color);

private:
  Glib::RefPtr<Gdk::Window> gdk_window_;

  Gtk::SizeRequestMode get_request_mode_vfunc() const override;

  void get_preferred_width_vfunc(int& minimum_width,
                                 int& natural_width) const override;

  void get_preferred_height_for_width_vfunc(int width,
                                            int& minimum_height,
                                            int& natural_height) const  override;

  void get_preferred_height_vfunc(int& minimum_height,
                                  int& natural_height) const override;

  void get_preferred_width_for_height_vfunc(int height,
                                            int& minimum_width,
                                            int& natural_width) const override;

  void on_size_allocate(Gtk::Allocation& allocation) override;
  void on_map() override;
  void on_unmap() override;
  void on_realize() override;
  void on_unrealize() override;
};

#endif//GMetronome_Pendulum_h
