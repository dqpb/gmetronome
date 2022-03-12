/*
 * Copyright (C) 2021 The GMetronome Team
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
#include "Meter.h"
#include "Ticker.h"

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
  virtual ~Pendulum();

  void setMeter(const Meter& meter);
  void setAction(ActionAngle angle);
  void setPhaseMode(PhaseMode mode);

  void synchronize(const audio::Ticker::Statistics& stats,
                   const std::chrono::microseconds& sync);
private:
  Meter meter_;
  double action_angle_;
  double phase_mode_shift_;
  bool animation_running_;
  double theta_;
  double alpha_;
  double omega_;
  double target_omega_;
  double target_theta_;
  std::chrono::microseconds last_frame_time_;
  double needle_amplitude_;
  double needle_theta_;
  double needle_length_;
  std::array<double,2> needle_base_;
  std::array<double,2> needle_tip_;
  double marking_radius_;
  double marking_amplitude_;

  void startAnimation();
  bool updateAnimation(const Glib::RefPtr<Gdk::FrameClock>&);

  Gdk::RGBA getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;
  Gdk::RGBA getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;

  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  void drawMarking(const Cairo::RefPtr<Cairo::Context>& cr);

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
