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
#include "Meter.h"

class Pendulum : public Gtk::Widget {

public:
  Pendulum();
  virtual ~Pendulum();

  void setMeter(const Meter& meter);  
  void scheduleClick(gint64 frame_time, double tempo, int accent);

private:
  Meter meter_;
  int animation_tick_callback_id_;
  double amplitude_;
  double theta_;
  double omega_;
  double alpha_;
  double phi_;
  double target_omega_;
  double target_phi_;
  gint64 last_frame_time_;
  
  void startAnimation();
  void stopAnimation();
  bool updateAnimation(const Glib::RefPtr<Gdk::FrameClock>&);

  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

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
