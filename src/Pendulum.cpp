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
#include <algorithm>
#include <iostream>

constexpr int    kPendulumHeight           = 150;
constexpr int    kPendulumWidth            = 300;

constexpr double kOmegaChangeRate          = (M_PI / 1000000) / 1000000;  // rad / us²
constexpr double kPhiChangeRate            = M_PI_2 / 1000000;       // rad / us

constexpr double kMaxAlpha                 = (M_PI / 1000000) / 1000000;  // rad / us²

Pendulum::Pendulum()
  : Glib::ObjectBase("pendulum"),
    Gtk::Widget(),
    meter_{kMeter_4_Simple},
    animation_tick_callback_id_{-1},
    amplitude_{0},
    theta_{0},
    omega_{0},
    alpha_{0},
    phi_{0},
    target_omega_{0},
    target_phi_{0},
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
  //std::cout << "time: " << frame_time << " tempo: " << tempo << " accent: " << accent << std::endl;
  
  //static int current_accent = -1;
  
  // if (accent != 0)
  //   return;

  // if (current_accent == accent)
  //   return;
  // else
  //   current_accent = accent;

  // target_omega_ = tempo / 60. / 1000000. * M_PI;

  // target_phi_ = std::fmod( - target_omega_ * frame_time, 2 * M_PI ) + (2 * M_PI);
  //target_phi_ = (2. * M_PI) - target_omega_ * frame_time;  

  double delta_theta = M_PI - std::fmod(theta_, M_PI);  
  gint64 delta_time = frame_time - g_get_monotonic_time();
  
  alpha_ = 2. * (delta_theta - delta_time * omega_) / (delta_time * delta_time);
  alpha_ = std::clamp(alpha_, -kMaxAlpha, kMaxAlpha);

  std::cout << " delta_theta: " << delta_theta
            << " delta_time: " << delta_time << std::endl;
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

    gint64 frame_time_delta = frame_time - last_frame_time_;

    if (frame_time_delta < 100000)
      return true;
    
    // if (target_omega_ > omega_)
    //   omega_ = std::min(target_omega_, omega_ + kOmegaChangeRate * frame_time_delta);
    // else
    //   omega_ = std::max(target_omega_, omega_ - kOmegaChangeRate * frame_time_delta);
    
    // if (target_phi_ > phi_)
    //   phi_ = std::min(target_phi_, phi_ + kPhiChangeRate * frame_time_delta);
    // else
    //   phi_ = std::max(target_phi_, phi_ - kPhiChangeRate * frame_time_delta);

    omega_ += alpha_ * frame_time_delta;
    
    theta_ += omega_ * frame_time_delta;
    theta_ = std::fmod(theta_, 2 * M_PI);    
    
    std::cout << "theta_: " << theta_ << "\t "
              << "omega_: " << omega_ << "\t "
              << "alpha_: " << alpha_;
    
    if (alpha_ == 0)
      std::cout << " (OK)" << std::endl;
    else
      std::cout << std::endl;

    // std::cout << "theta_: " << theta_ << "\t "
    //           << "omega_: " << omega_ << "\t "
    //           << "target_omega_: " << target_omega_ << " \t"
    //           << "phi_: " << phi_ << "\t "
    //           << "target_phi_: " << target_phi_;

    
    double max_amplitude = 1.;//(100. / 60. / 1000000. * M_PI) / omega_;
    amplitude_ = max_amplitude * std::sin(theta_);

    last_frame_time_ = frame_time;

    need_redraw = true;
  }

  if (need_redraw)
    queue_draw();

  return true;
}

bool Pendulum::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  const Gtk::Allocation allocation = get_allocation();
  const double w = (double)allocation.get_width() ;
  const double h = (double)allocation.get_height() ;

  auto refStyleContext = get_style_context();

  // paint the background
  // refStyleContext->render_background(cr,
  //                                    allocation.get_x(), allocation.get_y(),
  //                                    allocation.get_width(), allocation.get_height());

  // draw the foreground
  const auto state = refStyleContext->get_state();
  Gdk::Cairo::set_source_rgba(cr, refStyleContext->get_color(state));
  cr->move_to(0, 0);
  cr->line_to(w, 0);
  cr->line_to(w, h);
  cr->line_to(0, h);
  cr->line_to(0, 0);
  cr->stroke();

  //amplitude_ = -1;
  double L = (h - 10);
  
  cr->move_to(w/2, h);
  cr->line_to(w/2 - L * std::sin(amplitude_), h - L * std::cos(amplitude_));
  cr->stroke();
  
  return true;
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
