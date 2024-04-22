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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "TempoDisplay.h"
#include "Auxiliary.h"

#include <cmath>
#include <string>

using std::chrono::seconds;
using std::chrono::microseconds;

using std::literals::chrono_literals::operator""s;
using std::literals::chrono_literals::operator""us;

namespace {
  constexpr double kTriangleVelocity = 600.0; // degrees/s
}//unnamed namespace


void DividerLabel::TriangleMotionState::reset(double position)
{
  p_ = t_ = aux::math::modulo(position, 360.0);
  v_ = 0.0;
}

void DividerLabel::TriangleMotionState::moveTo(double target)
{
  t_ = aux::math::modulo(target, 360.0);
  if (t_ != p_ && v_ == 0.0)
  {
    if (g_random_boolean())
      v_ = kTriangleVelocity;
    else
      v_ = -kTriangleVelocity;
  }
}

bool DividerLabel::TriangleMotionState::step(const seconds_dbl& time)
{
  double delta_p = v_ * time.count();
  if (targetReached() || std::abs(delta_p) >= 360.0)
  {
    reset(t_);
  }
  else
  {
    double new_p = aux::math::modulo(p_ + delta_p, 360.0);
    double old_dist = aux::math::modulo(t_ - p_, 360.0);
    double new_dist = aux::math::modulo(t_ - new_p, 360.0);

    if ((v_ > 0.0 && new_dist >= old_dist) || (v_ < 0.0 && new_dist <= old_dist))
      reset(t_);
    else
      p_ = new_p;
  }
  return targetReached();
}

DividerLabel::DividerLabel()
{ }

DividerLabel::~DividerLabel()
{
  if (animation_running_)
    stopAnimation();
}

void DividerLabel::changeState(State new_state)
{
  if (new_state == state_)
    return;

  if (state_ == State::kStable || new_state == State::kStable)
  {
    if (animation_running_)
      stopAnimation();

    if (new_state == State::kSlowdown)
      triangle_motion_.reset(0.0);
    else if (new_state == State::kSpeedup)
      triangle_motion_.reset(180.0);

    state_ = new_state;
    Gtk::DrawingArea::queue_draw();
  }
  else if ((state_ == State::kSlowdown && new_state == State::kSpeedup) ||
           (state_ == State::kSpeedup && new_state == State::kSlowdown))
  {
    if (new_state == State::kSlowdown)
      triangle_motion_.moveTo(0.0);
    else
      triangle_motion_.moveTo(180.0);

    state_ = new_state;

    if (!animation_running_)
    {
      startAnimation();
    }
  }
}

void DividerLabel::startAnimation()
{
  if (animation_running_)
    return;

  animation_tick_callback_id_ = add_tick_callback(
    sigc::mem_fun(*this, &DividerLabel::updateAnimation) );

  last_frame_time_ = 0us;
  animation_running_ = true;
}

void DividerLabel::stopAnimation()
{
  if (!animation_running_)
    return;

  remove_tick_callback(animation_tick_callback_id_);
   animation_running_ = false;
}

bool DividerLabel::updateAnimation(const Glib::RefPtr<Gdk::FrameClock>& clock)
{
  if (clock)
  {
    microseconds frame_time {clock->get_frame_time()};
    if (last_frame_time_ > 0us)
    {
      seconds_dbl delta_time = frame_time - last_frame_time_;
      animation_running_ = !triangle_motion_.step(delta_time);
    }
    last_frame_time_ = frame_time;
  }

  Gtk::DrawingArea::queue_draw();
  return animation_running_;
}

void DividerLabel::updateSize()
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  Pango::FontDescription font = style_context->get_font(widget_state);

  auto pango_context = Gtk::DrawingArea::get_pango_context();
  auto pango_metrics = pango_context->get_metrics(font);

  int new_size = std::ceil(pango_metrics.get_approximate_digit_width() / (double) Pango::SCALE);

  if (new_size != content_size_)
  {
    content_size_ = new_size;
    Gtk::DrawingArea::queue_resize();
  }
}

void DividerLabel::on_style_updated()
{
  updateSize();
}

void DividerLabel::addBulletPath(const Cairo::RefPtr<Cairo::Context>& cr)
{
  cr->arc(0.0, 0.0, 0.5, 0.0, 2.0 * M_PI);
}

void DividerLabel::addTrianglePath(const Cairo::RefPtr<Cairo::Context>& cr)
{
  static constexpr double one_third = 1.0 / 3.0;
  static constexpr double two_third = 2.0 / 3.0;
  static constexpr double one_third_squared = one_third * one_third;
  static constexpr double two_third_squared = two_third * two_third;

  static const double side = 2.0 * std::sqrt(one_third_squared + two_third_squared);
  static const double side_half = side / 2.0;

  // we make the bottom side a bit shorter, so this is not exactly
  // an equilateral triangle
  static const double bottom_side_half = 0.70 * side_half;

  cr->move_to(0.0, two_third);
  cr->line_to(bottom_side_half, -one_third);
  cr->line_to(-bottom_side_half, -one_third);
  cr->line_to(0.0, two_third);
}

bool DividerLabel::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  Gdk::RGBA color = style_context->get_color(widget_state);

  Gdk::Cairo::set_source_rgba(cr, color);

  switch (state_) {
  case State::kSlowdown:
  case State::kSpeedup:
    cr->translate(Gtk::DrawingArea::get_width() / 2.0,
                  Gtk::DrawingArea::get_height() / 2.0);
    cr->rotate_degrees(triangle_motion_.position());
    cr->scale(content_size_ * 2.0 / 3.0, content_size_ * 2.0 / 3.0);
    addTrianglePath(cr);
    break;

  case State::kStable:
    [[fallthrough]];
  default:
    cr->translate(Gtk::DrawingArea::get_width() / 2.0,
                  Gtk::DrawingArea::get_height() / 2.0);
    cr->scale(content_size_ * 2.0 / 5.0, content_size_ * 2.0 / 5.0);
    addBulletPath(cr);
    break;
  };

  cr->fill();

  return false;
}

Gtk::SizeRequestMode DividerLabel::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
}

void DividerLabel::get_preferred_width_vfunc(int& minimum_width,
                                             int& natural_width) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_width = natural_width = content_size_ + margin.get_left() + margin.get_right();
}

void DividerLabel::get_preferred_height_for_width_vfunc(int width,
                                                        int& minimum_height,
                                                        int& natural_height) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_height = natural_height = content_size_ + margin.get_top() + margin.get_bottom();
}

void DividerLabel::get_preferred_width_for_height_vfunc(int height,
                                                        int& minimum_width,
                                                        int& natural_width) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_width = natural_width = content_size_ + margin.get_left() + margin.get_right();
}

void DividerLabel::get_preferred_height_vfunc(int& minimum_height,
                                              int& natural_height) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_height = natural_height = content_size_ + margin.get_top() + margin.get_bottom();
}

NumericLabel::NumericLabel(int number, std::size_t digits, bool fill)
  : number_{number},
    kDigits{digits},
    kFill{fill},
    digits_{kDigits}
{
  Gtk::DrawingArea::set_can_focus(false);

  updateDigits();
}

void NumericLabel::display(int number)
{
  if (number == number_)
    return;

  number_ = number;
  updateDigits();

  Gtk::DrawingArea::queue_draw();
}

void NumericLabel::updateDigits()
{
  std::string s = std::to_string(number_);

  auto s_it = s.rbegin();
  for (auto& digit : digits_)
  {
    if (s_it != s.rend())
      digit = *s_it++;
    else if (kFill)
      digit = '0';
    else
      digit.clear();
  }
}

void NumericLabel::updateDigitDimensions()
{
  auto pango_context = Gtk::DrawingArea::get_pango_context();

  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(pango_context);

  digit_width_ = 0;
  digit_height_ = 0;

  for (int i=0; i<=9; ++i)
  {
    layout->set_text(Glib::ustring::format(i));
    Pango::Rectangle ink_ext = layout->get_pixel_ink_extents();

    digit_width_ = std::max(digit_width_, ink_ext.get_width());
    digit_height_ = std::max(digit_height_, ink_ext.get_height());
  }

}

void NumericLabel::on_style_updated()
{
  Gtk::DrawingArea::on_style_updated();

  updateDigitDimensions();
  queue_resize();
}

void NumericLabel::on_screen_changed(const Glib::RefPtr<Gdk::Screen>& previous_screen)
{
  Gtk::DrawingArea::on_screen_changed(previous_screen);

  updateDigitDimensions();
  queue_resize();
}

bool NumericLabel::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  Pango::FontDescription font = style_context->get_font(widget_state);
  Gdk::RGBA color = style_context->get_color(widget_state);

  auto pango_context = Gtk::DrawingArea::get_pango_context();
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(pango_context);

  auto margin = style_context->get_margin(widget_state);

  double x_offset = Gtk::DrawingArea::get_width() - margin.get_right();
  double y_offset = margin.get_top();

  static Gdk::RGBA kNegativeColor {"rgba(255, 0, 0, 1.0)"};

  if (number_ < 0)
    Gdk::Cairo::set_source_rgba(cr, kNegativeColor);
  else
    Gdk::Cairo::set_source_rgba(cr, color);

  for (unsigned d = 0; d < kDigits; ++d)
  {
    if (!digits_[d].empty())
    {
      layout->set_text(digits_[d]);

      Pango::Rectangle ink_ext = layout->get_pixel_ink_extents();

      // shift to next digit
      x_offset -= digit_width_;

      double x = x_offset - ink_ext.get_x() + (digit_width_ - ink_ext.get_width()) / 2.0;
      double y = y_offset - ink_ext.get_y() + (digit_height_ - ink_ext.get_height()) / 2.0;

      cr->move_to(x, y);

      layout->show_in_cairo_context(cr);
    }
  }

  return false;
}

Gtk::SizeRequestMode NumericLabel::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
}

void NumericLabel::get_preferred_width_vfunc(int& minimum_width,
                                             int& natural_width) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_width = natural_width = kDigits * digit_width_ + margin.get_left() + margin.get_right();
}

void NumericLabel::get_preferred_height_for_width_vfunc(int width,
                                                        int& minimum_height,
                                                        int& natural_height) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_height = natural_height = digit_height_ + margin.get_top() + margin.get_bottom();
}

void NumericLabel::get_preferred_width_for_height_vfunc(int height,
                                                        int& minimum_width,
                                                        int& natural_width) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_width = natural_width = kDigits * digit_width_ + margin.get_left() + margin.get_right();
}

void NumericLabel::get_preferred_height_vfunc(int& minimum_height,
                                              int& natural_height) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_height = natural_height = digit_height_ + margin.get_top() + margin.get_bottom();
}


TempoDisplay::TempoDisplay()
{
  Gtk::Box::set_direction(Gtk::TEXT_DIR_LTR);
  Gtk::Box::set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  Gtk::Box::set_spacing(0);

  Gtk::Box::pack_start(integral_label_, Gtk::PACK_EXPAND_WIDGET);
  integral_label_.set_direction(Gtk::TEXT_DIR_LTR);
  integral_label_.set_halign(Gtk::ALIGN_END);
  integral_label_.set_valign(Gtk::ALIGN_CENTER);
  integral_label_.set_name("tempoIntegralLabel");

  Gtk::Box::pack_end(fraction_label_, Gtk::PACK_EXPAND_WIDGET);
  fraction_label_.set_direction(Gtk::TEXT_DIR_LTR);
  fraction_label_.set_halign(Gtk::ALIGN_START);
  fraction_label_.set_valign(Gtk::ALIGN_CENTER);
  fraction_label_.set_name("tempoFractionLabel");

  Gtk::Box::set_center_widget(divider_label_);
  divider_label_.set_direction(Gtk::TEXT_DIR_LTR);
  divider_label_.set_halign(Gtk::ALIGN_CENTER);
  divider_label_.set_valign(Gtk::ALIGN_CENTER);
  divider_label_.set_name("tempoDividerLabel");

  Gtk::Box::show_all();
}

void TempoDisplay::display(double tempo, double accel)
{
  if (tempo != tempo_)
  {
    static const int precision = 2;

    double tempo_integral;
    double tempo_fraction;

    tempo_fraction = std::modf(tempo, &tempo_integral);

    int tempo_integral_int = tempo_integral;
    int tempo_fraction_int = std::round(tempo_fraction * std::pow(10, precision));

    if (tempo_fraction_int == 100)
    {
      tempo_fraction_int = 0;
      tempo_integral_int += 1;
    }

    integral_label_.display(tempo_integral_int);
    fraction_label_.display(tempo_fraction_int);
  }

  if (accel != accel_)
  {
    if (accel == 0)
      divider_label_.changeState(DividerLabel::State::kStable);
    else if (accel > 0)
      divider_label_.changeState(DividerLabel::State::kSpeedup);
    else
      divider_label_.changeState(DividerLabel::State::kSlowdown);
  }

  tempo_ = tempo;
  accel_ = accel;
}
