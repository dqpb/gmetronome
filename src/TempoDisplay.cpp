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
#include <array>

namespace
{
  const Glib::ustring kBlinkClassName = "blink";
  constexpr double kDimAlpha = 0.07;
}//unnamed namespace

NumericLabel::NumericLabel(std::size_t digits, int number, bool fill, bool dim)
  : kDigits_{digits},
    number_{number},
    kDefaultFill_{fill},
    kDefaultDim_{dim},
    fill_{kDefaultFill_},
    dim_{kDefaultDim_},
    digits_{kDigits_}
{
  Gtk::DrawingArea::set_can_focus(false);

  updateDigits();
}

void NumericLabel::display(int number, bool fill, bool dim)
{
  if (!unset_ && number == number_ && fill_ == fill && dim_ == dim)
    return;

  unset_ = false;
  number_ = number;
  fill_ = fill;
  dim_ = dim;

  updateDigits();
  Gtk::DrawingArea::queue_draw();
}

void NumericLabel::reset(bool fill, bool dim)
{
  if (unset_ && fill_ == fill && dim_ == dim)
    return;

  unset_ = true;
  number_ = 0;
  fill_ = fill;
  dim_ = dim;

  updateDigits();
  Gtk::DrawingArea::queue_draw();
}

void NumericLabel::updateDigits()
{
  std::string s = std::to_string( (number_ < 0) ? -number_ : number_);
  if (unset_)
    n_fill_ = kDigits_;
  else
    n_fill_ = kDigits_ - s.size();

  auto s_it = s.rbegin();
  for (auto& digit : digits_)
  {
    if (s_it != s.rend())
      digit = *s_it++;
    else if (fill_)
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
  Gdk::RGBA text_color = style_context->get_color(widget_state);

  Gdk::RGBA dim_color = text_color;
  dim_color.set_alpha(kDimAlpha);

  auto pango_context = Gtk::DrawingArea::get_pango_context();
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(pango_context);

  auto margin = style_context->get_margin(widget_state);

  double x_offset = Gtk::DrawingArea::get_width() - margin.get_right();
  double y_offset = margin.get_top();

  static Gdk::RGBA kNegativeColor {"rgba(255, 0, 0, 1.0)"};

  for (unsigned d = 0; d < kDigits_; ++d)
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

      if (dim_ && d >= kDigits_ - n_fill_)
        Gdk::Cairo::set_source_rgba(cr, dim_color);
      else if (number_ < 0)
        Gdk::Cairo::set_source_rgba(cr, kNegativeColor);
      else
        Gdk::Cairo::set_source_rgba(cr, text_color);

      layout->show_in_cairo_context(cr);
      // style_context->render_layout(cr, x, y, layout);
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

  minimum_width = natural_width = kDigits_ * digit_width_ + margin.get_left() + margin.get_right();
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

  minimum_width = natural_width = kDigits_ * digit_width_ + margin.get_left() + margin.get_right();
}

void NumericLabel::get_preferred_height_vfunc(int& minimum_height,
                                              int& natural_height) const
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  auto margin = style_context->get_margin(widget_state);

  minimum_height = natural_height = digit_height_ + margin.get_top() + margin.get_bottom();
}

void StatusIcon::switchImage(StatusIcon::Image id)
{
  if (id == id_)
    return;

  switch (id) {

  case Image::kContinuousUp:
    set_from_icon_name("gm-curve-linear-up-symbolic", size_);
    break;

  case Image::kContinuousDown:
    set_from_icon_name("gm-curve-linear-down-symbolic", size_);
    break;

  case Image::kStepwiseUp:
    set_from_icon_name("gm-curve-stepwise-up-symbolic", size_);
    break;

  case Image::kStepwiseDown:
    set_from_icon_name("gm-curve-stepwise-down-symbolic", size_);
    break;

  case Image::kTargetHit:
    set_from_icon_name("gm-target-hit-symbolic", size_);
    break;

  case Image::kSync:
    set_from_icon_name("gm-snd-bell-symbolic", size_);
    break;

  case Image::kNone:
    [[fallthrough]];

  default:
    Gtk::Image::clear();
    break;
  };

  id_ = id;
}

void StatusIcon::enableBlink()
{
  if (blink_)
    return;

  if (auto style_context = get_style_context(); style_context)
  {
    if (!style_context->has_class(kBlinkClassName))
      style_context->add_class(kBlinkClassName);
  }
  blink_ = true;
}

void StatusIcon::disableBlink()
{
  if (!blink_)
    return;

  if (auto style_context = get_style_context(); style_context)
  {
    if (style_context->has_class(kBlinkClassName))
      style_context->remove_class(kBlinkClassName);
  }
  blink_ = false;
}

LCD::LCD()
{
  Gtk::Box::set_orientation(Gtk::ORIENTATION_VERTICAL);
  Gtk::Box::set_spacing(0);

  // configure stat box
  stat_box_.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  stat_box_.set_vexpand(true);

  stat_box_.pack_start(beat_label_, Gtk::PACK_EXPAND_WIDGET);
  beat_label_.set_name("beatLabel");
  beat_label_.set_halign(Gtk::ALIGN_START);
  beat_label_.set_valign(Gtk::ALIGN_CENTER);

  stat_box_.pack_end(status_icon_, Gtk::PACK_SHRINK);
  status_icon_.set_name("statusIcon");
  status_icon_.set_halign(Gtk::ALIGN_END);
  status_icon_.set_valign(Gtk::ALIGN_CENTER);

  stat_box_.pack_end(hold_label_, Gtk::PACK_SHRINK);
  hold_label_.set_name("holdLabel");
  hold_label_.set_halign(Gtk::ALIGN_END);
  hold_label_.set_valign(Gtk::ALIGN_CENTER);

  tempo_frac_label_.set_name("tempoFracLabel");
  tempo_frac_label_.set_halign(Gtk::ALIGN_START);
  tempo_frac_label_.set_valign(Gtk::ALIGN_CENTER);
  tempo_frac_label_.reset(true, true);

  if (get_direction() == Gtk::TEXT_DIR_RTL)
  {
    tempo_frac_label_.set_halign(Gtk::ALIGN_END);
    stat_box_.pack_start(tempo_frac_label_, Gtk::PACK_EXPAND_WIDGET);
  }
  else
  {
    tempo_frac_label_.set_halign(Gtk::ALIGN_START);
    stat_box_.pack_end(tempo_frac_label_, Gtk::PACK_EXPAND_WIDGET);
  }

  stat_box_.set_center_widget(tempo_int_label_);
  tempo_int_label_.set_name("tempoIntLabel");
  tempo_int_label_.set_halign(Gtk::ALIGN_CENTER);
  tempo_int_label_.set_valign(Gtk::ALIGN_CENTER);
  tempo_int_label_.zero();

  stat_box_.show_all();

  // configure profile label
  profile_label_.set_name("profileLabel");
  profile_label_.set_lines(1);
  profile_label_.set_line_wrap(false);
  profile_label_.set_ellipsize(Pango::ELLIPSIZE_END);
  profile_label_.set_hexpand(false);
  profile_label_.set_max_width_chars(20);

  Gtk::Box::pack_start(stat_box_, Gtk::PACK_EXPAND_WIDGET);
  Gtk::Box::pack_start(profile_label_, Gtk::PACK_SHRINK);

  Gtk::Box::show_all();

  Gtk::Settings::get_default()->property_gtk_theme_name().signal_changed()
    .connect(sigc::mem_fun(*this, &LCD::onThemeNameChanged));

  LCD::property_parent().signal_changed()
    .connect(sigc::mem_fun(*this, &LCD::onParentChanged));
}

std::pair<int, int> LCD::decomposeTempo(double tempo)
{
    static const double kPrecisionPow10 = std::pow(10.0, kPrecision);

    double tempo_int;
    double tempo_frac;

    tempo_frac = std::modf(tempo, &tempo_int);

    int tempo_int_int = tempo_int;
    int tempo_frac_int = std::round(tempo_frac * kPrecisionPow10);

    if (tempo_frac_int == kPrecisionPow10)
    {
      tempo_frac_int = 0;
      tempo_int_int += 1;
    }

    return {tempo_int_int, tempo_frac_int};
}

void LCD::updateStatistics(const audio::Ticker::Statistics& stats)
{
  if (stats.generator == audio::kRegularGenerator)
  {
    if (!stats.default_meter)
    {
      int beat =  stats.accent / stats.division + 1;
      beat_label_.display(beat);
    }
    else
      beat_label_.reset();

    auto [tempo_int, tempo_frac] = decomposeTempo(stats.tempo);
    tempo_int_label_.display(tempo_int);

    if (tempo_frac != 0 || stats.mode == audio::Ticker::AccelMode::kContinuous || stats.syncing)
    {
      tempo_frac_label_.display(tempo_frac);
      status_icon_.switchImage(StatusIcon::Image::kNone);
      hold_label_.reset();
    }
    else
      tempo_frac_label_.reset(true, true);

    if (stats.mode == audio::Ticker::AccelMode::kContinuous)
    {
      if (stats.tempo < stats.target)
        status_icon_.switchImage(StatusIcon::Image::kContinuousUp);
      else if (stats.tempo > stats.target)
        status_icon_.switchImage(StatusIcon::Image::kContinuousDown);
      else
        status_icon_.switchImage(StatusIcon::Image::kTargetHit);

      hold_label_.reset();
    }
    else if (stats.mode == audio::Ticker::AccelMode::kStepwise)
    {
      if (stats.tempo < stats.target)
      {
        hold_label_.display(stats.hold);
        status_icon_.switchImage(StatusIcon::Image::kStepwiseUp);
      }
      else if (stats.tempo > stats.target)
      {
        hold_label_.display(stats.hold);
        status_icon_.switchImage(StatusIcon::Image::kStepwiseDown);
      }
      else
      {
        hold_label_.reset();
        status_icon_.switchImage(StatusIcon::Image::kTargetHit);
      }
    }
    else
    {
      hold_label_.reset();
      status_icon_.switchImage(StatusIcon::Image::kNone);
    }

    if (stats.pending && status_icon_.image() != StatusIcon::Image::kTargetHit)
      status_icon_.enableBlink();
    else
      status_icon_.disableBlink();
  }
  else
  {
    beat_label_.reset();
    tempo_int_label_.zero();
    tempo_frac_label_.reset(true, true);
    hold_label_.reset();
    status_icon_.switchImage(StatusIcon::Image::kNone);
  }
}

void LCD::setProfileTitle(const Glib::ustring& title, bool is_placeholder)
{
  auto style_context = profile_label_.get_style_context();
  if (is_placeholder)
  {
    if (!style_context->has_class("placeholder"))
      style_context->add_class("placeholder");
  }
  else if (style_context->has_class("placeholder"))
    style_context->remove_class("placeholder");

  profile_label_.set_text(title);

  if (!profile_label_.is_visible())
    profile_label_.show();
}

void LCD::unsetProfileTitle()
{
  profile_label_.hide();
}

Gdk::RGBA LCD::getBGColor(const Gtk::Widget* widget)
{
  Gdk::RGBA color;

  if (widget == nullptr)
    return color;

  // To determinie a background color of the headerbar we render the background
  // into a Cairo::ImageSurface and compute the average RGB values over all pixels.
  // In this way we get a meaninful color, even if the theme uses background images,
  // gradients etc.

  static constexpr int kSurfaceWidth  = 50;
  static constexpr int kSurfaceHeight = 50;
  static constexpr int kNumPixels     = kSurfaceWidth * kSurfaceHeight;

  auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                             kSurfaceWidth,
                                             kSurfaceHeight);

  auto cairo_context = Cairo::Context::create(surface);

  if (auto style_context = widget->get_style_context(); style_context)
  {
    style_context->set_state(Gtk::STATE_FLAG_NORMAL);
    style_context->render_background(cairo_context, 0, 0, kSurfaceWidth, kSurfaceHeight);

    const unsigned char* data = surface->get_data();

    std::array<double,4> rgba {0.0, 0.0, 0.0, 0.0};

    for (int row = 0; row < kSurfaceHeight; ++row)
    {
      for (int col = 0; col < kSurfaceWidth; ++col)
      {
        int index = row * surface->get_stride() + col * 4;
        rgba[0] += data[index + 0];
        rgba[1] += data[index + 1];
        rgba[2] += data[index + 2];
        rgba[3] += data[index + 3];
      }
    }
    rgba[0] /= (double) kNumPixels * 255.0;
    rgba[1] /= (double) kNumPixels * 255.0;
    rgba[2] /= (double) kNumPixels * 255.0;
    rgba[3] /= (double) kNumPixels * 255.0;

    color.set_rgba(rgba[0], rgba[1], rgba[2], rgba[3]);
  }

  return color;
}

Gdk::RGBA LCD::getFGColor(const Gtk::Widget* widget)
{
  Gdk::RGBA color;

  if (widget == nullptr)
    return color;

  if (auto style_context = widget->get_style_context(); style_context)
  {
    style_context->set_state(Gtk::STATE_FLAG_NORMAL);
    color = style_context->get_color(Gtk::STATE_FLAG_NORMAL);
  }

  return color;
}

void LCD::updateCSSClass()
{
  Gtk::Container* parent = LCD::get_parent();

  if (parent == nullptr)
    return;

  Gdk::RGBA bg_color = getBGColor(LCD::get_parent());
  Gdk::RGBA fg_color = getFGColor(LCD::get_parent());

  double bg_lum =
    0.2126 * bg_color.get_red()
    + 0.7152 * bg_color.get_green()
    + 0.0722 * bg_color.get_blue();

  double fg_lum =
    0.2126 * fg_color.get_red()
    + 0.7152 * fg_color.get_green()
    + 0.0722 * fg_color.get_blue();

  if (auto style_context = get_style_context(); style_context)
  {
    if (fg_lum < bg_lum) {
      // switch to light theme
      if (!style_context->has_class("light-theme"))
        style_context->add_class("light-theme");
      if (style_context->has_class("dark-theme"))
        style_context->remove_class("dark-theme");
    }
    else if (fg_lum > bg_lum) {
      // switch to dark theme
      if (!style_context->has_class("dark-theme"))
        style_context->add_class("dark-theme");
      if (style_context->has_class("light-theme"))
        style_context->remove_class("light-theme");
    }
    else {
      // no theme (due to error in get*Color)
      if (style_context->has_class("light-theme"))
        style_context->remove_class("light-theme");
      if (style_context->has_class("dark-theme"))
        style_context->remove_class("dark-theme");
    }
  }
}

void LCD::onThemeNameChanged()
{ updateCSSClass(); }

void LCD::onParentChanged()
{ updateCSSClass(); }
