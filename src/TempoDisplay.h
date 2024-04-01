/*
 * Copyright (C) 2024 The GMetronome Team
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

#ifndef GMetronome_TempoDisplay_h
#define GMetronome_TempoDisplay_h

#include <gtkmm.h>
#include <vector>
#include <chrono>

class DividerLabel : public Gtk::DrawingArea {
public:
  enum class State
  {
    kSpeedup,
    kSlowdown,
    kStable
  };

public:
  DividerLabel();
  ~DividerLabel();

  void changeState(State s);

private:
  State state_{State::kStable};
  int content_size_{1};
  guint animation_tick_callback_id_{0};
  bool animation_running_{false};

  using seconds_dbl = std::chrono::duration<double>;

  class TriangleMotionState {
  public:
    void reset(double position = 0.0);
    void moveTo(double target);
    bool step(const seconds_dbl& time);

    double position() const
      { return p_; }
    bool targetReached() const
      { return p_ == t_; }

  private:
    double p_{0.0}; // position (degreed)
    double v_{0.0}; // velocity (degrees/s)
    double t_{0.0}; // target position
  };

  TriangleMotionState triangle_motion_;

  std::chrono::microseconds last_frame_time_{0};

  void startAnimation();
  void stopAnimation();
  bool updateAnimation(const Glib::RefPtr<Gdk::FrameClock>& clock);

  void updateSize();

  void addBulletPath(const Cairo::RefPtr<Cairo::Context>& cr);
  void addTrianglePath(const Cairo::RefPtr<Cairo::Context>& cr);

  // default signal handler
  void on_style_updated() override;
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
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
};

class NumericLabel : public Gtk::DrawingArea {
public:
  NumericLabel(int number = 0, std::size_t digits = 3, bool fill = false);

  void display(int number);

  void clear()
    { display(0); }

  int number() const
    { return number_; }
  std::size_t digits() const
    { return kDigits; }
  bool fill() const
    { return kFill; }

private:
  int number_{0};
  const std::size_t kDigits;
  const bool kFill;

  std::vector<Glib::ustring> digits_;

  int digit_width_{0};
  int digit_height_{0};

  void updateDigits();
  void updateDigitDimensions();

private:
  // default signal handler
  void on_style_updated() override;
  void on_screen_changed(const Glib::RefPtr< Gdk::Screen > &previous_screen) override;
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
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
};

class TempoDisplay : public Gtk::Box {
public:
  TempoDisplay();

  void display(double tempo, double accel);

  double tempo() const
    { return tempo_; }
  double accel() const
    { return accel_; }

private:
  double tempo_{0.0};
  double accel_{0.0};
  DividerLabel divider_label_;
  NumericLabel integral_label_{0,3,false};
  NumericLabel fraction_label_{0,2,true};
};

#endif//GMetronome_TempoDisplay_h
