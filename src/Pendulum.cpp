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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Pendulum.h"
#include <cairomm/context.h>
#include <algorithm>

#ifndef NDEBUG
# include <iostream>
#endif

namespace {

  using std::chrono::seconds;
  using std::chrono::microseconds;

  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""us;

  using seconds_dbl = std::chrono::duration<double>;

  // behaviour
  constexpr double kActionAngleReal     = M_PI / 5.5;  // rad
  constexpr double kActionAngleCenter   = 0.0;         // rad
  constexpr double kActionAngleEdge     = M_PI / 2.0;  // rad
  constexpr double kPhaseModeShiftLeft  = 0.0;         // rad
  constexpr double kPhaseModeShiftRight = M_PI;        // rad

  constexpr seconds_dbl kSyncTime       = 750ms;
  constexpr seconds_dbl kShutdownTime   = 2000ms;

  // dynamics
  constexpr double kMaxOmega           = 250.0 / 60.0 * M_PI;   // 250 bpm in rad/s
  constexpr double kMinNeedleAmplitude = M_PI / 6.0;            // rad
  constexpr double kMaxNeedleAmplitude = M_PI / 4.0;            // rad
  constexpr double kNeedleAmplitudeChangeRate  = 0.8 * M_PI;    // rad/s
  constexpr double kDialAmplitudeChangeRate    = 2.0 * M_PI;    // rad/s

  // element appearance
  constexpr double kNeedleWidth        = 3.0;   // pixel
  constexpr double kNeedleShadowOffset = 6.0;   // pixel
  constexpr double kNeedleLength       = 92.0;  // percent of dial radius
  constexpr double kKnobRadius         = 10.0;  // pixel

  // widget dimensions
  const double kWidgetWidthHeightRatio = 2.0 * std::sin(kMaxNeedleAmplitude);
  const int    kWidgetHeight           = 150;
  const int    kWidgetWidth            = kWidgetWidthHeightRatio * kWidgetHeight;

  //helper to compute the maximum needle amplitude for a given angular velocity
  constexpr double needleAmplitude(double velocity)
  {
    constexpr double kRatio = - (kMaxNeedleAmplitude - kMinNeedleAmplitude) / kMaxOmega;
    return kRatio * velocity + kMaxNeedleAmplitude;
  }

}//unnamed namespace

Pendulum::Pendulum()
  : Glib::ObjectBase("pendulum"),
    Gtk::Widget(),
    action_angle_{kActionAngleReal},
    phase_mode_shift_{kPhaseModeShiftLeft},
    dial_amplitude_{kMaxNeedleAmplitude}
{}

void Pendulum::setAction(ActionAngle action)
{
  switch (action)
  {
  case ActionAngle::kCenter:
    action_angle_ = kActionAngleCenter;
    break;
  case ActionAngle::kEdge:
    action_angle_ = kActionAngleEdge;
    break;
  case ActionAngle::kReal:
    [[fallthrough]];
  default:
    action_angle_ = kActionAngleReal;
    break;
  };
}

void Pendulum::setPhaseMode(PhaseMode mode)
{
  phase_mode_ = mode;
}

void Pendulum::togglePhase()
{
  if (phase_mode_shift_ == kPhaseModeShiftLeft)
    phase_mode_shift_ = kPhaseModeShiftRight;
  else
    phase_mode_shift_ = kPhaseModeShiftLeft;
}

void Pendulum::start()
{
  switch (phase_mode_)
  {
  case PhaseMode::kRight:
    phase_mode_shift_ = kPhaseModeShiftRight;
    break;
  case PhaseMode::kLeft:
    [[fallthrough]];
  default:
    phase_mode_shift_ = kPhaseModeShiftLeft;
    break;
  };

  beat_pos_ = 0.0;

  if (state_ == kStop)
  {
    k_.reset(phase_mode_shift_, 0.0);
    startAnimation();
  }
  state_ = kStartup;
}

void Pendulum::stop()
{
  k_.shutdown(kShutdownTime);
  target_omega_ = 0.0;
  state_ = kShutdown;
}

void Pendulum::synchronize(const audio::Ticker::Statistics& stats,
                           const std::chrono::microseconds& sync)
{
  if (state_ == kStop || state_ == kShutdown)
    return;

  if (stats.generator_state == audio::kFillBufferGenerator)
  {
    if (state_ != kFillBuffer)
    {
      state_ = kFillBuffer;
    }
  }
  else if (stats.generator_state == audio::kRegularGenerator)
  {
    if (state_ != kRegular) // init kinematics
    {
      double amplitude = std::max(needle_amplitude_, kMinNeedleAmplitude);

      double new_theta = std::asin(needle_theta_ / amplitude);
      needle_amplitude_ = amplitude;

      double start_theta = phase_mode_shift_;
      double alt_theta = M_PI - new_theta;

      double dist = std::abs(std::remainder(start_theta - new_theta, 2.0 * M_PI));
      double alt_dist = std::abs(std::remainder(start_theta - alt_theta, 2.0 * M_PI));

      if (alt_dist < dist)
        k_.reset(alt_theta, k_.omega());
      else
        k_.reset(new_theta, k_.omega());

      state_ = kRegular;
    }

    target_omega_ = stats.tempo / 60.0 * M_PI;

    double omega_dev = 0.0;
    double theta_dev = 0.0;

    omega_dev = target_omega_ - k_.omega();

    double old_beat_pos = beat_pos_;
    beat_pos_ = stats.position;

    double displacement = aux::math::modulo(beat_pos_ - old_beat_pos, 1.0);
    beat_pos_ = std::fmod(old_beat_pos + displacement, 2.0);

    double target_theta = M_PI * beat_pos_;

    microseconds now(g_get_monotonic_time());
    microseconds click_time = stats.timestamp + stats.backend_latency + sync;
    seconds_dbl time_delta = now - click_time;

    target_theta += target_omega_ * time_delta.count();
    target_theta += action_angle_;
    target_theta += phase_mode_shift_;

    double theta_dist = std::remainder(target_theta - k_.theta(), 2.0 * M_PI);

    theta_dev = omega_dev * kSyncTime.count() + theta_dist;

    k_.synchronize(theta_dev, omega_dev, kSyncTime);
  }
}

void Pendulum::startAnimation()
{
  last_frame_time_ = 0us;
  add_tick_callback(sigc::mem_fun(*this, &Pendulum::updateAnimation));
}

bool Pendulum::updateAnimation(const Glib::RefPtr<Gdk::FrameClock>& clock)
{
  if (clock)
  {
    microseconds frame_time {0};

    auto timings = clock->get_current_timings();
    if (timings)
    {
      frame_time = microseconds(timings->get_predicted_presentation_time());

      if (frame_time.count() == 0)
        frame_time = microseconds(timings->get_presentation_time());
    }

    // no timings or (predicted) presentation time available
    if (frame_time.count() == 0)
      frame_time = microseconds(clock->get_frame_time());

    if (frame_time == last_frame_time_)
      return true;

    seconds_dbl frame_time_delta = (frame_time - last_frame_time_);

    if (frame_time_delta > 0.5s)
    {
      last_frame_time_ = frame_time;
      return true;
    }

    last_frame_time_ = frame_time;

    k_.step(frame_time_delta);

    bool redraw_dial = false;

    double dial_target_amplitude
      = (state_ != kRegular) ? needleAmplitude(0.0) : needleAmplitude(target_omega_);

    if (std::abs(dial_target_amplitude - dial_amplitude_) > 0.001)
    {
      dial_amplitude_ += kDialAmplitudeChangeRate
        * std::tanh(dial_target_amplitude - dial_amplitude_) * frame_time_delta.count();
      redraw_dial = true;
    }

    double needle_target_amplitude = dial_target_amplitude;

    if (state_ != kRegular)
      needle_target_amplitude = 0.0;

    needle_amplitude_ += kNeedleAmplitudeChangeRate
      * std::tanh(needle_target_amplitude - needle_amplitude_) * frame_time_delta.count();

    needle_theta_ = needle_amplitude_ * std::sin( k_.theta() );

    auto old_needle_tip = needle_tip_;
    needle_tip_[0] = needle_base_[0] - needle_length_ * std::sin(needle_theta_);
    needle_tip_[1] = needle_base_[1] - needle_length_ * std::cos(needle_theta_);

    int x, y, w, h;

    if(redraw_dial)
    {
      x = needle_base_[0] - needle_length_;
      y = 0;
      w = 2.0 * needle_length_;
      h = get_allocated_height();
    }
    else
    {
      x = std::min(needle_base_[0], std::min(old_needle_tip[0], needle_tip_[0])) - kNeedleWidth;
      y = std::min(old_needle_tip[1], needle_tip_[1]) - kNeedleWidth;
      w = std::max(needle_base_[0], std::max(old_needle_tip[0], needle_tip_[0])) - x + kNeedleWidth;
      h = needle_base_[1] - y + kNeedleWidth;
    };

    queue_draw_area(x,y,w,h);
  }

  double center_deviation = std::abs(std::remainder(needle_theta_, M_PI));

  bool continue_animation =
    state_ >= kStartup || std::abs(k_.omega()) > 0.0001 || center_deviation > 0.0001;

  if (!continue_animation)
    state_ = kStop;

  return continue_animation;
}

Gdk::RGBA Pendulum::getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state);

  return color;
}

Gdk::RGBA Pendulum::getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state | Gtk::STATE_FLAG_LINK);
  return color;
}

bool Pendulum::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();

  // draw the foreground
  Gdk::RGBA primary_color = getPrimaryColor(style_context);
  Gdk::RGBA secondary_color = getSecondaryColor(style_context);

  draw_dial(cr, primary_color);
  draw_needle(cr, primary_color);
  draw_knob(cr, primary_color);

  return true;
}

void Pendulum::draw_dial(const Cairo::RefPtr<Cairo::Context>& cr,
                         Gdk::RGBA dial_color)
{
  dial_color.set_alpha(0.5);

  static constexpr double three_pi_half = 3.0 * M_PI / 2.0;
  const double sin_dial_amplitude = std::sin(dial_amplitude_);
  const double cos_dial_amplitude = std::cos(dial_amplitude_);
  const double needle_length_half = needle_length_ / 2.0;

  // draw dial
  cr->save();
  cr->move_to(needle_base_[0] - (needle_length_half) * sin_dial_amplitude,
              needle_base_[1] - (needle_length_half) * cos_dial_amplitude);

  cr->line_to(needle_base_[0] - dial_radius_ * sin_dial_amplitude,
              needle_base_[1] - dial_radius_ * cos_dial_amplitude);

  cr->arc(needle_base_[0],
          needle_base_[1],
          dial_radius_,
          three_pi_half - dial_amplitude_,
          three_pi_half + dial_amplitude_);

  cr->line_to(needle_base_[0] + (needle_length_half) * sin_dial_amplitude,
              needle_base_[1] - (needle_length_half) * cos_dial_amplitude);

  cr->arc_negative(needle_base_[0],
                   needle_base_[1],
                   needle_length_half,
                   three_pi_half + dial_amplitude_,
                   three_pi_half - dial_amplitude_);

  cr->set_source_rgba(dial_color.get_red(),
                      dial_color.get_green(),
                      dial_color.get_blue(),
                      0.05);
  cr->fill_preserve();

  Gdk::Cairo::set_source_rgba(cr, dial_color);
  cr->set_line_width(1.0);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->stroke();

  cr->move_to(needle_base_[0], needle_base_[1]);
  cr->line_to(needle_base_[0], needle_base_[1] - dial_radius_);

  static const std::vector<double> dash_pattern({4.0, 4.0});
  cr->set_dash(dash_pattern, 0);
  cr->stroke();
  cr->restore();
}

void Pendulum::draw_needle(const Cairo::RefPtr<Cairo::Context>& cr,
                           const Gdk::RGBA& needle_color)
{
  static const Gdk::RGBA shadow_color("rgba(0,0,0,.1)");

  // draw needle shadow
  Gdk::Cairo::set_source_rgba(cr, shadow_color);
  cr->set_line_width(kNeedleWidth);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base_[0], needle_base_[1]);
  cr->line_to(needle_tip_[0], needle_tip_[1] + kNeedleShadowOffset);
  cr->stroke();

  // draw needle
  Gdk::Cairo::set_source_rgba(cr, needle_color);
  cr->set_line_width(kNeedleWidth);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base_[0], needle_base_[1]);
  cr->line_to(needle_tip_[0], needle_tip_[1]);
  cr->stroke();

}

void Pendulum::draw_knob(const Cairo::RefPtr<Cairo::Context>& cr,
                         const Gdk::RGBA& knob_color)
{
  // draw knob
  Gdk::Cairo::set_source_rgba(cr, knob_color);
  cr->arc(needle_base_[0], needle_base_[1], kKnobRadius, 0.0, 2.0 * M_PI);
  cr->fill_preserve();
}

Gtk::SizeRequestMode Pendulum::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

void Pendulum::get_preferred_width_vfunc(int& minimum_width,
                                         int& natural_width) const
{
  minimum_width = kWidgetWidth;
  natural_width = kWidgetWidth;
}

void Pendulum::get_preferred_height_for_width_vfunc(int width,
                                                    int& minimum_height,
                                                    int& natural_height) const
{
  minimum_height = width / kWidgetWidthHeightRatio;
  natural_height = width / kWidgetWidthHeightRatio;
}

void Pendulum::get_preferred_height_vfunc(int& minimum_height,
                                          int& natural_height) const
{
  minimum_height = kWidgetHeight;
  natural_height = kWidgetHeight;
}

void Pendulum::get_preferred_width_for_height_vfunc(int height,
                                                    int& minimum_width,
                                                    int& natural_width) const
{
  minimum_width = kWidgetWidthHeightRatio * height;
  natural_width = kWidgetWidthHeightRatio * height;
}

void Pendulum::on_size_allocate(Gtk::Allocation& allocation)
{
  int x = allocation.get_x();
  int y = allocation.get_y();
  int width = allocation.get_width();
  int height = allocation.get_height();

  // use the offered allocation for this widget
  set_allocation(allocation);

  if(gdk_window_)
    gdk_window_->move_resize(x, y, width, height);

  dial_radius_ = std::min(width / (2.0 * std::sin(needleAmplitude(0.0))), (double)height);
  dial_radius_ = std::floor(dial_radius_ - 1.0) + 0.5;

  needle_length_ = std::round(dial_radius_ / 100.0 * kNeedleLength);
  needle_base_[0] = std::floor(width / 2.0) + 0.5; // prevent blurred middle line
  needle_base_[1] = std::floor((height + dial_radius_) / 2.0) + 1.5;
  needle_tip_[0] = needle_base_[0] - needle_length_ * std::sin(needle_theta_);
  needle_tip_[1] = needle_base_[1] - needle_length_ * std::cos(needle_theta_);
}

void Pendulum::on_map()
{
  Gtk::Widget::on_map();
}

void Pendulum::on_unmap()
{
  Gtk::Widget::on_unmap();
}

void Pendulum::on_realize()
{
  set_realized();

  if(!gdk_window_)
  {
    // create the GdkWindow:
    GdkWindowAttr attributes;
    memset(&attributes, 0, sizeof(attributes));

    Gtk::Allocation allocation = get_allocation();

    // set initial position and size of the Gdk::Window:
    attributes.x = allocation.get_x();
    attributes.y = allocation.get_y();
    attributes.width = allocation.get_width();
    attributes.height = allocation.get_height();

    attributes.event_mask = get_events () | Gdk::EXPOSURE_MASK;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;

    gdk_window_ = Gdk::Window::create(get_parent_window(), &attributes,
                                      GDK_WA_X | GDK_WA_Y);
    set_window(gdk_window_);

    // make the widget receive expose events
    gdk_window_->set_user_data(gobj());
  }
}

void Pendulum::on_unrealize()
{
  gdk_window_.reset();
  Gtk::Widget::on_unrealize();
}
