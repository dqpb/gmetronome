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

constexpr int    kPendulumHeight    = 150;
constexpr int    kPendulumWidth     = 300;
constexpr double kOmegaChangeRate   = (5. * M_PI / 1.) ;  // rad / s²

Pendulum::Pendulum()
  : Glib::ObjectBase("pendulum"),
    Gtk::Widget(),
    meter_{kMeter_4_Simple},
    animation_tick_callback_id_{-1},
    theta_{0},
    omega_{0},
    alpha_{0},
    target_omega_{0},
    target_theta_{0},
    last_frame_time_{0}
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

void Pendulum::scheduleClick(gint64 frame_time, double tempo, int accent)
{
  double now = g_get_monotonic_time() / 1000000.;
  double click_time = frame_time / 1000000.;

  target_omega_ = tempo / 60. * M_PI;

  if (accent % 2 == 1)
    target_theta_ = M_PI - (target_omega_ * click_time - target_omega_ * now);
  else
    target_theta_ = (2 * M_PI) - (target_omega_ * click_time - target_omega_ * now);
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

    if (frame_time_delta > 1.0)
    {
      last_frame_time_ = frame_time;
      return true;
    }

    if (frame_time_delta > 0.015)
    {
      alpha_ = kOmegaChangeRate * std::tanh(target_omega_ - omega_);
      alpha_ += kOmegaChangeRate * std::sin(target_theta_ - theta_);

      alpha_ = std::clamp(alpha_, -kOmegaChangeRate, kOmegaChangeRate);
      
      omega_ += alpha_ * frame_time_delta;

      theta_ += omega_ * frame_time_delta;
      theta_ = std::fmod(theta_, 2 * M_PI);

      // std::cout << "omega: " << omega_
      //           << " theta: " << theta_
      //           << " target_omega: " << target_omega_
      //           << " target_theta: " << target_theta_
      //           << " alpha: " << alpha_
      //           << std::endl;

      last_frame_time_ = frame_time;
      need_redraw = true;
    }
  }

  if (need_redraw)
  {
    int min = std::min(get_allocated_width(), get_allocated_height());
    //queue_draw();
    queue_draw_area(get_allocated_width() / 2. - min,
                    (get_allocated_height() - min) / 2,
                    2*min,
                    min);
  }
  return true;
}

bool Pendulum::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  const Gtk::Allocation allocation = get_allocation();
  const double width = (double)allocation.get_width() ;
  const double height = (double)allocation.get_height() ;
  const double min_size = std::min(width,height);

  auto refStyleContext = get_style_context();
  
  // draw the foreground
  const auto state = refStyleContext->get_state();

  Gdk::RGBA needle_color = refStyleContext->get_color(state);

  Gdk::RGBA shadow_color;
  shadow_color.set_rgba(0, 0, 0, .1);

  Gdk::RGBA highlight_color;
  highlight_color.set_rgba(1, 1, 1, .6);

  Gdk::RGBA marking_color = refStyleContext->get_color(state);
  marking_color.set_alpha(.5);
  
  //double max_amplitude = 1. - (omega_ / 20.);
  double kNeedleMaxAmplitude = M_PI / 3.5;
  double needle_max_amplitude = kNeedleMaxAmplitude - (omega_ / 25);
  double needle_amplitude = needle_max_amplitude * std::sin( theta_ );
  double needle_length = std::min(width / (2. * std::sin(kNeedleMaxAmplitude)), height - 30);
  
  double needle_base[2] = { width / 2., (height + min_size) / 2. };
  double needle_tip[2] = {
    needle_base[0] - needle_length * std::sin(needle_amplitude),
    needle_base[1] - needle_length * std::cos(needle_amplitude)
  };
  
  // debug: draw a frame 
  Gdk::Cairo::set_source_rgba(cr, refStyleContext->get_color(state));
  cr->move_to(0, 0);
  cr->line_to(width, 0);
  cr->line_to(width, height);
  cr->line_to(0, height);
  cr->line_to(0, 0);
  cr->stroke();
  
  // draw markings
  cr->save();
  Gdk::Cairo::set_source_rgba(cr, marking_color);
  cr->set_line_width(1.);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  
  cr->arc(needle_base[0],
          needle_base[1],
          needle_length + 10.,
          -M_PI / 2. - needle_max_amplitude,
          -M_PI / 2. + needle_max_amplitude);

  cr->stroke();

  cr->move_to(needle_base[0], needle_base[1]);
  cr->line_to(needle_base[0], needle_base[1] - needle_length - 10);
  cr->set_dash(std::vector<double>({4.,4.}),0);
  cr->stroke();
  cr->restore();
  
  // draw needle shadow
  Gdk::Cairo::set_source_rgba(cr, shadow_color);
  cr->set_line_width(3.);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base[0], needle_base[1] + 5);
  cr->line_to(needle_tip[0], needle_tip[1] + 5);
  cr->stroke();

  // draw needle
  Gdk::Cairo::set_source_rgba(cr, needle_color);
  cr->set_line_width(3.);
  cr->set_line_cap(Cairo::LINE_CAP_ROUND);
  cr->move_to(needle_base[0], needle_base[1]);
  cr->line_to(needle_tip[0], needle_tip[1]);
  cr->stroke();

  // draw knob
  Gdk::Cairo::set_source_rgba(cr, needle_color);
  cr->arc(needle_base[0], needle_base[1], 10., 0, 2. * M_PI);
  cr->fill_preserve();

  // auto knob_light_gradient = Cairo::LinearGradient::create(0, needle_base[1] - 5, 0, needle_base[1] + 5);
  // knob_light_gradient->add_color_stop_rgba(
  //   0.0,
  //   highlight_color.get_red(),
  //   highlight_color.get_green(),
  //   highlight_color.get_blue(),
  //   0.8
  //   );

  // knob_light_gradient->add_color_stop_rgba(0.5, 0.0, 0.0, 0.0, 0.0);

  // knob_light_gradient->add_color_stop_rgba(
  //   1.0,
  //   shadow_color.get_red(),
  //   shadow_color.get_green(),
  //   shadow_color.get_blue(),
  //   0.8
  //   );

  // cr->set_source(knob_light_gradient);
  // cr->fill();

  // Gdk::Cairo::set_source_rgba(cr, needle_color);
  // cr->arc(needle_base[0], needle_base[1], 8., 0, 2. * M_PI);
  // cr->fill_preserve();

  return true;
}

void Pendulum::drawMarking(const Cairo::RefPtr<Cairo::Context>& cr)
{
  
}

Gtk::SizeRequestMode Pendulum::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
}

void Pendulum::get_preferred_width_vfunc(int& minimum_width,
                                         int& natural_width) const
{
  minimum_width = kPendulumWidth;
  natural_width = kPendulumWidth;
}

void Pendulum::get_preferred_height_for_width_vfunc(int width,
                                                    int& minimum_height,
                                                    int& natural_height) const
{
  minimum_height = kPendulumHeight;
  natural_height = kPendulumHeight;
}

void Pendulum::get_preferred_height_vfunc(int& minimum_height,
                                          int& natural_height) const
{
  minimum_height = kPendulumHeight;
  natural_height = kPendulumHeight;
}

void Pendulum::get_preferred_width_for_height_vfunc(int height,
                                                    int& minimum_width,
                                                    int& natural_width) const
{
  minimum_width = kPendulumWidth;
  natural_width = kPendulumWidth;
}

void Pendulum::on_size_allocate(Gtk::Allocation& allocation)
{
  //Do something with the space that we have actually been given:
  //(We will not be given heights or widths less than we have requested, though
  //we might get more)

  //Use the offered allocation for this container:
  set_allocation(allocation);

  if(gdk_window_)
  {
    gdk_window_->move_resize( allocation.get_x(), allocation.get_y(),
                              allocation.get_width(), allocation.get_height() );
  }
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
  //Do not call base class Gtk::Widget::on_realize().
  //It's intended only for widgets that set_has_window(false).

  set_realized();

  //Get the themed style from the CSS file:
  // m_scale = m_scale_prop.get_value();
  // std::cout << "m_scale (example_scale from the theme/css-file) is: "
  //           << m_scale << std::endl;

  if(!gdk_window_)
  {
    //Create the GdkWindow:

    GdkWindowAttr attributes;
    memset(&attributes, 0, sizeof(attributes));

    Gtk::Allocation allocation = get_allocation();

    //Set initial position and size of the Gdk::Window:
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

    //make the widget receive expose events
    gdk_window_->set_user_data(gobj());
  }
}

void Pendulum::on_unrealize()
{
  gdk_window_.reset();

  //Call base class:
  Gtk::Widget::on_unrealize();
}
