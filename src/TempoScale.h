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

#ifndef GMetronome_TempoScale_h
#define GMetronome_TempoScale_h

#include <gtkmm.h>
#include <tuple>
#include <vector>

/**
 * @class TempoRange
 */
class TempoRange {
public:
  explicit TempoRange(double min = 0.0, double max = 0.0);
  TempoRange(const std::tuple<double, double>& range);

  // implicit conversion
  operator std::tuple<double, double>() const
    { return {min_, max_}; }

  double min() const
    { return min_; }
  double max() const
    { return max_; }

  static bool validate(double min, double max);

  static bool validate(const std::tuple<double, double>& range)
    { return validate(std::get<0>(range), std::get<1>(range)); }

private:
  double min_{0.0};
  double max_{0.0};
};

/**
 * @lass Marking
 *
 */
class Marking {
public:
  Marking(const TempoRange& range, const Glib::ustring& label);

  void setTempoRange(const TempoRange& range);

  void setLabel(const Glib::ustring& label);

  const TempoRange& range() const
    { return range_; }
  const Glib::ustring& label() const
    { return label_;}

private:
  TempoRange range_;
  Glib::ustring label_;
};

/**
 * @class TempoScale
 *
 */
class TempoScale : public Gtk::Scale {
public:
  // ctor
  TempoScale();

  void setMarkings(std::vector<Marking> markings);

  const std::vector<Marking>& markings() const
    { return markings_; }

private:
  std::vector<Marking> markings_;

  Gdk::RGBA getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;
  Gdk::RGBA getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const;

private:
  // Glib::RefPtr<Gdk::Window> gdk_window_;

  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

  // Gtk::SizeRequestMode get_request_mode_vfunc() const override;

  // void get_preferred_width_vfunc(int& minimum_width,
  //                                int& natural_width) const override;

  // void get_preferred_height_for_width_vfunc(int width,
  //                                           int& minimum_height,
  //                                           int& natural_height) const  override;

  // void get_preferred_height_vfunc(int& minimum_height,
  //                                 int& natural_height) const override;

  // void get_preferred_width_for_height_vfunc(int height,
  //                                           int& minimum_width,
  //                                           int& natural_width) const override;

  // void on_size_allocate(Gtk::Allocation& allocation) override;
  // void on_map() override;
  // void on_unmap() override;
  // void on_realize() override;
  // void on_unrealize() override;
};

#endif//GMetronome_TempoScale_h
