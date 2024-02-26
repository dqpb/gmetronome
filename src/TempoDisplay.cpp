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
#include <cmath>
#include <limits>

NumericLabel::NumericLabel(unsigned number, unsigned digits, bool fill)
  : number_{std::numeric_limits<unsigned>::max()},
    fill_{fill}
{
  while (digits-- > 0)
  {
    auto label = Gtk::manage(new Gtk::Label);

    label->set_text("");
    label->set_use_markup(false);
    label->set_width_chars(1);
    label->set_max_width_chars(1);
    label->set_lines(1);
    label->set_line_wrap(false);
    label->set_selectable(false);
    label->set_single_line_mode(true);
    label->show();

    Gtk::Box::pack_start(*label, Gtk::PACK_SHRINK);
  }

  Gtk::Box::set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  Gtk::Box::set_homogeneous(true);
  Gtk::Box::set_spacing(0);

  setNumber(number);
}

void NumericLabel::setNumber(unsigned number)
{
  if (number == number_)
    return;

  number_ = number;

  auto children = Gtk::Box::get_children();
  bool first = true;

  for (auto it = children.rbegin(); it != children.rend(); ++it)
  {
    auto label = static_cast<Gtk::Label*>(*it);

    if (number > 0)
        label->set_text(Glib::ustring::format(number % 10));
    else if (fill_ || first)
      label->set_text("0");
    else
      label->set_text("");

    number /= 10;
    first = false;
  }
}

namespace {
  const Glib::ustring kAccelUpSymbol     = "\xe2\x96\xb4"; // "▴"
  const Glib::ustring kAccelDownSymbol   = "\xe2\x96\xbe"; // "▾"
  const Glib::ustring kAccelStableSymbol = "\xe2\x80\xa2"; // "•"
}//unnamed namespace

TempoDisplay::TempoDisplay()
{
  Gtk::Box::pack_start(integral_label_, Gtk::PACK_EXPAND_WIDGET);
  integral_label_.set_halign(Gtk::ALIGN_END);
  integral_label_.set_valign(Gtk::ALIGN_CENTER);
  integral_label_.set_name("tempoIntegralLabel");
  integral_label_.show();

  Gtk::Box::pack_end(fraction_label_, Gtk::PACK_EXPAND_WIDGET);
  fraction_label_.set_halign(Gtk::ALIGN_START);
  fraction_label_.set_valign(Gtk::ALIGN_CENTER);
  fraction_label_.set_name("tempoFractionLabel");
  fraction_label_.show();

  Gtk::Box::set_center_widget(divider_label_);
  divider_label_.set_text(kAccelStableSymbol);
  divider_label_.set_use_markup(false);
  divider_label_.set_width_chars(1);
  divider_label_.set_max_width_chars(1);
  divider_label_.set_lines(1);
  divider_label_.set_line_wrap(false);
  divider_label_.set_selectable(false);
  divider_label_.set_single_line_mode(true);
  divider_label_.set_name("tempoDividerLabel");
  divider_label_.show();

  Gtk::Box::set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  Gtk::Box::set_spacing(0);
}

void TempoDisplay::setTempo(double tempo, double accel)
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

    integral_label_.setNumber(tempo_integral_int);
    fraction_label_.setNumber(tempo_fraction_int);
  }

  if (accel != accel_)
  {
    if (accel == 0)
      divider_label_.set_text(kAccelStableSymbol);
    else if (accel > 0)
      divider_label_.set_text(kAccelUpSymbol);
    else
      divider_label_.set_text(kAccelDownSymbol);
  }

  tempo_ = tempo;
  accel_ = accel;
}
