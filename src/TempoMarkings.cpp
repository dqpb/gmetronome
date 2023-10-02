/*
 * Copyright (C) 2023 The GMetronome Team
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

#include "TempoMarkings.h"
#include "Error.h"

#include <cmath>

TempoRange::TempoRange(double min, double max)
  : min_{min}, max_{max}
{
  if (!validate(min_, max_))
    throw GMetronomeError {"Invalid tempo range"};
}

TempoRange::TempoRange(const std::tuple<double, double>& range)
  : TempoRange(std::get<0>(range), std::get<1>(range))
{
  // nothing
}

bool TempoRange::validate(double min, double max)
{
   return std::isfinite(min)
     && !std::isnan(max)
     && min >= 0.0
     && max >= min;
}

Gdk::RGBA TempoMarkings::getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state);

  return color;
}

Gdk::RGBA TempoMarkings::getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state | Gtk::STATE_FLAG_LINK);
  return color;
}

Cairo::RefPtr<Cairo::ImageSurface> createScaleSurface()
{
  int surface_width = 300;
  int surface_height = 20;
    
  auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                             surface_width,
                                             surface_height);

  auto cr = Cairo::Context::create(surface);

  //Gdk::RGBA primary_color = getPrimaryColor(style_context);

  cr->move_to(0.0, surface_height - 0.0);
  cr->line_to(300.0, surface_height - 0.0);
  cr->stroke();

  int m = 2.0;
  
  std::vector<double> dash5 = {1.0, 4.0 * m + m - 1};
  std::vector<double> dash10 = {1.0, 9.0 * m + m - 1};
  std::vector<double> dash50 = {1.0, 49.0 * m + m - 1};

  cr->set_dash(dash5, 0.0);
  cr->move_to(0.0, surface_height - 1.0);
  cr->line_to(300.0, surface_height - 1.0);
  cr->stroke();

  cr->set_dash(dash10, 0.0);
  cr->move_to(0.0, surface_height - 2.0);
  cr->line_to(300.0, surface_height - 2.0);
  cr->stroke();

  cr->set_dash(dash50, 0.0);
  cr->move_to(0.0, surface_height - 3.0);
  cr->line_to(300.0, surface_height - 3.0);
  cr->stroke();

  cr->set_dash(dash50, 0.0);
  cr->move_to(0.0, surface_height - 4.0);
  cr->line_to(300.0, surface_height - 4.0);
  cr->stroke();

  return surface;
}


bool TempoMarkings::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();

  // draw the foreground
  Gdk::RGBA primary_color = getPrimaryColor(style_context);
  Gdk::RGBA secondary_color = getSecondaryColor(style_context);

  const Gtk::Allocation allocation = get_allocation();

  auto front_pattern = Cairo::LinearGradient::create(0.0, 0.0, allocation.get_width(), 0.0);

  front_pattern->add_color_stop_rgba(0.0, 0.0, 0.0, 0.0, 0.0);
  front_pattern->add_color_stop_rgba(0.5, 0.0, 0.0, 0.0, 0.5);
  front_pattern->add_color_stop_rgba(1.0, 0.0, 0.0, 0.0, 0.0);

  auto source_surface = createScaleSurface();
  
  cr->set_source(source_surface, 0.0, 0.0);

  cr->mask(front_pattern);

  return true;
}


Gtk::SizeRequestMode TempoMarkings::get_request_mode_vfunc() const
{
  //Accept the default value supplied by the base class.
  return Gtk::Widget::get_request_mode_vfunc();
}

void TempoMarkings::get_preferred_width_vfunc(int& minimum_width,
                                              int& natural_width) const
{
  minimum_width = 60;
  natural_width = 100;
}

void TempoMarkings::get_preferred_height_for_width_vfunc(int width,
                                                         int& minimum_height,
                                                         int& natural_height) const
{
  minimum_height = 20;
  natural_height = 20;
}

void TempoMarkings::get_preferred_height_vfunc(int& minimum_height,
                                               int& natural_height) const
{
  minimum_height = 20;
  natural_height = 20;
}

void TempoMarkings::get_preferred_width_for_height_vfunc(int height,
                                                         int& minimum_width,
                                                         int& natural_width) const
{
  minimum_width = 60;
  natural_width = 100;
}

void TempoMarkings::on_size_allocate(Gtk::Allocation& allocation)
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

void TempoMarkings::on_map()
{
  // call base class:
  Gtk::Widget::on_map();
}

void TempoMarkings::on_unmap()
{
  // call base class:
  Gtk::Widget::on_unmap();
}

void TempoMarkings::on_realize()
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

    gdk_window_ = Gdk::Window::create(get_parent_window(), &attributes, GDK_WA_X | GDK_WA_Y);

    set_window(gdk_window_);

    //make the widget receive expose events
    gdk_window_->set_user_data(gobj());
  }
}

void TempoMarkings::on_unrealize()
{
  gdk_window_.reset();

  // call base class:
  Gtk::Widget::on_unrealize();
}
