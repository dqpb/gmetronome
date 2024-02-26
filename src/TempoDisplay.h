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

class NumericLabel : public Gtk::Box {
public:
  NumericLabel(unsigned number = 0, unsigned digits = 3, bool fill = false);

  void setNumber(unsigned number);

  unsigned getNumber() const
    { return number_; }
  unsigned getDigits() const
    { return Gtk::Box::get_children().size(); }
  bool getFill() const
    { return fill_; }

private:
  unsigned number_;
  bool fill_;
};


class TempoDisplay : public Gtk::Box {
public:
  TempoDisplay();

  void setTempo(double tempo, double accel);

  double getTempo() const
    { return tempo_; }
  double getAccel() const
    { return accel_; }

private:
  double tempo_{0.0};
  double accel_{0.0};
  Gtk::Label divider_label_;
  NumericLabel integral_label_{0,3,false};
  NumericLabel fraction_label_{0,2,true};
};

#endif//GMetronome_TempoDisplay_h
