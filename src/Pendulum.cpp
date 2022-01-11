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

#include "Pendulum.h"
#include <cairomm/context.h>
#include <algorithm>
#include <iostream>

// animation
constexpr double kAnimationFrameRate = 65.;  // frames/s

// behaviour
constexpr double kClickActionAngle   = M_PI / 8.0;  // rad

// needle dynamics
constexpr double kMaxAlpha           = (8.0 * M_PI / 1.0);    // rad/s²
constexpr double kMinOmega           =  30.0 / 60.0 * M_PI;   // rad/s
constexpr double kMaxOmega           = 250.0 / 60.0 * M_PI;   // rad/s
constexpr double kMinNeedleAmplitude = M_PI / 6.0;            // rad
constexpr double kMaxNeedleAmplitude = M_PI / 3.5;            // rad
constexpr double kNeedleAmplitudeChangeRate = 1.5 * M_PI;     // rad/s

// element appearance
constexpr double kNeedleWidth        = 3.0;   // pixel
constexpr double kNeedleShadowOffset = 6.0;   // pixel
constexpr double kNeedleLength       = 92.0;  // percent of markings height
constexpr double kKnobRadius         = 10.0;  // pixel

// widget dimensions
constexpr double kWidgetWidthHeightRatio = 2.0 * std::sin(kMaxNeedleAmplitude);
constexpr int    kWidgetHeight           = 150;
constexpr int    kWidgetWidth            = kWidgetWidthHeightRatio * kWidgetHeight;

Pendulum::Pendulum()
  : Glib::ObjectBase("pendulum"),
    Gtk::Widget(),
    meter_{kMeter_4_Simple},
    animation_tick_callback_id_{-1},
    theta_{0},
    alpha_{0},
    omega_{0},
    omega_view_{0},
    target_omega_{0},
    target_theta_{0},
    last_frame_time_{0},
    needle_amplitude_{kMaxNeedleAmplitude},
    needle_theta_{0.0},
    needle_length_{0.9},
    needle_base_ {0.5, 1.0},
    needle_tip_ {0.5, 0.0},
    marking_radius_{1.0}
{
  startAnimation();
}

Pendulum::~Pendulum()
{
  stopAnimation();
}

void Pendulum::setMeter(const Meter& meter)
{
  meter_ = meter;
}

void Pendulum::synchronize(const audio::Ticker::Statistics& stats)
{
  if (stats.current_beat < 0)
  {
    target_omega_ = 0;
    target_theta_ = 0;
    return;
  }

  audio::microseconds now(g_get_monotonic_time());

  double beat = (now * stats.current_beat) / (stats.timestamp + stats.backend_latency);

  target_omega_ = stats.current_tempo / 60. * M_PI;
  target_theta_ = beat * M_PI;
  target_theta_ += kClickActionAngle;
  target_theta_ = std::fmod(target_theta_ + 2. * M_PI, 2. * M_PI);
}

void Pendulum::startAnimation()
{
  if (animation_tick_callback_id_ >= 0)
    return;

  animation_tick_callback_id_ = add_tick_callback(
    sigc::mem_fun(*this, &Pendulum::updateAnimation) );
}

void Pendulum::stopAnimation()
{
  if (animation_tick_callback_id_ < 0)
    return;

  remove_tick_callback(animation_tick_callback_id_);
  animation_tick_callback_id_ = -1;
}

//helper to compute the maximum needle amplitude for a given angular velocity
constexpr double needleAmplitude(double velocity)
{
  constexpr double kRatio = - (kMaxNeedleAmplitude - kMinNeedleAmplitude) / kMaxOmega;
  return kRatio * velocity + kMaxNeedleAmplitude;
}

bool Pendulum::updateAnimation(const Glib::RefPtr<Gdk::FrameClock>& clock)
{
  bool need_redraw = false;

  if (clock)
  {
    gint64 frame_time = 0;

    auto timings = clock->get_current_timings();
    if (timings)
    {
      frame_time = timings->get_predicted_presentation_time();

      if (frame_time == 0)
        frame_time = timings->get_presentation_time();
    }

    // no timings or (predicted) presentation time available
    if (frame_time == 0)
      frame_time = clock->get_frame_time();

    double frame_time_delta = (frame_time - last_frame_time_) / 1000000.;

    if (frame_time_delta > .5)
    {
      last_frame_time_ = frame_time;
      return true;
    }

    if (frame_time_delta > 1.0 / kAnimationFrameRate)
    {
      // The acceleration of the needle (alpha) is influenced by the deviation
      // of the current needle velocity (omega) from the target velocity and the
      // deviation of the current needle phase (theta) from the target phase.
      alpha_  = kMaxAlpha * std::tanh(target_omega_ - omega_);
      alpha_ += kMaxAlpha * std::sin(target_theta_ - theta_);

      // update needle velocity (rad/s)
      omega_ += alpha_ * frame_time_delta;

      // update needle position (rad)
      theta_ += omega_ * frame_time_delta;
      theta_ = std::fmod(theta_, 2 * M_PI);

      target_theta_ += target_omega_ * frame_time_delta;

      last_frame_time_ = frame_time;

      need_redraw = true;

      if (need_redraw)
      {
        double target_amplitude = needleAmplitude(target_omega_);

        auto old_needle_amplitude = needle_amplitude_;
        needle_amplitude_ += kNeedleAmplitudeChangeRate
          * std::tanh(target_amplitude - needle_amplitude_) * frame_time_delta;

        needle_theta_ = needle_amplitude_ * std::sin( theta_ );

        auto old_needle_tip = needle_tip_;

        needle_tip_[0] = needle_base_[0] - needle_length_ * std::sin(needle_theta_);
        needle_tip_[1] = needle_base_[1] - needle_length_ * std::cos(needle_theta_);

        int x, y, w, h;

        if(old_needle_amplitude != needle_amplitude_)
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
    }
  }
  return true;
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
  const Gtk::Allocation allocation = get_allocation();
  const double width = (double)allocation.get_width() ;
  const double height = (double)allocation.get_height() ;

  auto style_context = get_style_context();

  // draw the foreground
  Gdk::RGBA primary_color = getPrimaryColor(style_context);
  Gdk::RGBA secondary_color = getSecondaryColor(style_context);
  Gdk::RGBA needle_color = primary_color;

  static const Gdk::RGBA shadow_color("rgba(0,0,0,.1)");
  static const Gdk::RGBA highlight_color("rgb(255,255,255,.6)");

  Gdk::RGBA marking_color = primary_color;
  marking_color.set_alpha(.5);

  static const double three_pi_half = 3.0 * M_PI / 2.0;
  const double sin_needle_amplitude = std::sin(needle_amplitude_);
  const double cos_needle_amplitude = std::cos(needle_amplitude_);
  const double needle_length_half = needle_length_ / 2.;

  // debug: draw a frame
  // Gdk::Cairo::set_source_rgba(cr, primary_color);
  // cr->move_to(0, 0);
  // cr->line_to(width, 0);
  // cr->line_to(width, height);
  // cr->line_to(0, height);
  // cr->line_to(0, 0);
  // cr->stroke();

  // draw markings
  cr->save();
  cr->move_to(needle_base_[0] - (needle_length_half) * sin_needle_amplitude,
              needle_base_[1] - (needle_length_half) * cos_needle_amplitude);

  cr->line_to(needle_base_[0] - marking_radius_ * sin_needle_amplitude,
              needle_base_[1] - marking_radius_ * cos_needle_amplitude);

  cr->arc(needle_base_[0],
          needle_base_[1],
          marking_radius_,
          three_pi_half - needle_amplitude_,
          three_pi_half + needle_amplitude_);

  cr->line_to(needle_base_[0] + (needle_length_half) * sin_needle_amplitude,
              needle_base_[1] - (needle_length_half) * cos_needle_amplitude);

  cr->arc_negative(needle_base_[0],
                   needle_base_[1],
                   needle_length_half,
                   three_pi_half + needle_amplitude_,
                   three_pi_half - needle_amplitude_);

  cr->set_source_rgba(marking_color.get_red(),
                      marking_color.get_green(),
                      marking_color.get_blue(),
                      0.05);
  cr->fill_preserve();

  Gdk::Cairo::set_source_rgba(cr, marking_color);
  cr->set_line_width(1.);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->stroke();

  cr->move_to(needle_base_[0], needle_base_[1]);
  cr->line_to(needle_base_[0], needle_base_[1] - marking_radius_);
  cr->set_dash(std::vector<double>({4.,4.}),0);
  cr->stroke();
  cr->restore();

  // draw needle shadow
  Gdk::Cairo::set_source_rgba(cr, shadow_color);
  cr->set_line_width(3.);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base_[0], needle_base_[1] + kNeedleShadowOffset);
  cr->line_to(needle_tip_[0], needle_tip_[1] + kNeedleShadowOffset);
  cr->stroke();

  // draw needle
  Gdk::Cairo::set_source_rgba(cr, needle_color);
  cr->set_line_width(kNeedleWidth);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base_[0], needle_base_[1]);
  cr->line_to(needle_tip_[0], needle_tip_[1]);
  cr->stroke();

  // draw knob
  Gdk::Cairo::set_source_rgba(cr, needle_color);
  cr->arc(needle_base_[0], needle_base_[1], kKnobRadius, 0.0, 2.0 * M_PI);
  cr->fill_preserve();

  return true;
}

void Pendulum::drawMarking(const Cairo::RefPtr<Cairo::Context>& cr)
{}

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
  int min_size = std::min(width, height);

  // use the offered allocation for this widget
  set_allocation(allocation);

  if(gdk_window_)
    gdk_window_->move_resize(x, y, width, height);

  marking_radius_ = std::min(width / (2.0 * std::sin(needleAmplitude(0.0))), (double)height);
  marking_radius_ = std::floor(marking_radius_ - 1.0) + 0.5;

  needle_length_ = std::round(marking_radius_ / 100.0 * kNeedleLength);
  needle_base_[0] = std::floor(width / 2.0) + 0.5; // prevent blurred middle line
  needle_base_[1] = std::floor((height + marking_radius_) / 2.0) + 1.5;
  needle_tip_[0] = needle_base_[0];
  needle_tip_[1] = needle_base_[1] - needle_length_;
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
  // Do not call base class Gtk::Widget::on_realize().
  // It's intended only for widgets that set_has_window(false).

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

  //Call base class:
  Gtk::Widget::on_unrealize();
}
