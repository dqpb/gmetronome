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

#include "Ticker.h"

#include <gtkmm.h>
#include <vector>
#include <chrono>

class NumericLabel : public Gtk::DrawingArea {
public:
  NumericLabel(std::size_t digits = 3, int number = 0, bool fill = false, bool dim = false);

  void display(int number, bool fill, bool dim);
  void display(int number)
    { display(number, kDefaultFill_, kDefaultDim_); }

  void zero(bool fill, bool dim)
    { display(0, fill, dim); }
  void zero()
    { display(0); }

  void reset(bool fill, bool dim);
  void reset()
    { reset(kDefaultFill_, kDefaultDim_); }

  int number() const
    { return number_; }
  std::size_t digits() const
    { return kDigits_; }
  bool unset() const
    { return unset_; }

private:
  const std::size_t kDigits_;
  int number_{0};
  const bool kDefaultFill_;
  const bool kDefaultDim_;
  bool fill_;
  bool dim_;

  std::vector<Glib::ustring> digits_;

  bool unset_{true};
  int n_fill_{0};
  int digit_width_{0};
  int digit_height_{0};

  void updateDigits();
  void updateDigitDimensions();

private:
  // default signal handler
  void on_style_updated() override;
  void on_screen_changed(const Glib::RefPtr< Gdk::Screen > &previous_screen) override;
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  Gtk::SizeRequestMode get_request_mode_vfunc() const override;
  void get_preferred_width_vfunc(int& minimum_width,
                                 int& natural_width) const override;
  void get_preferred_height_for_width_vfunc(int width,
                                            int& minimum_height,
                                            int& natural_height) const override;
  void get_preferred_height_vfunc(int& minimum_height,
                                  int& natural_height) const override;
  void get_preferred_width_for_height_vfunc(int height,
                                            int& minimum_width,
                                            int& natural_width) const override;
};

class StatusIcon : public Gtk::Image {
public:
  enum class Image
  {
    kNone,
    kContinuousUp,
    kContinuousDown,
    kStepwiseUp,
    kStepwiseDown,
    kTargetHit,
    kSync
  };

  StatusIcon() = default;

  void switchImage(StatusIcon::Image id);
  Image image() const
    { return id_; }

  void enableBlink();
  void disableBlink();
  bool isBlinking() const
    { return blink_; }

private:
  StatusIcon::Image id_{Image::kNone};
  Gtk::IconSize size_{Gtk::ICON_SIZE_SMALL_TOOLBAR};
  bool blink_{false};
};

class LCD : public Gtk::Box {
public:
  LCD();

  void updateStatistics(const audio::Ticker::Statistics& stats);

  void setProfileTitle(const Glib::ustring& title, bool is_placeholder);

  void unsetProfileTitle();

private:
  Gtk::Box     stat_box_;
  Gtk::Label   profile_label_;
  NumericLabel beat_label_{2, 0, true, true};
  NumericLabel tempo_int_label_{3, 0, true, true};

  static constexpr int kPrecision = 2;
  NumericLabel tempo_frac_label_{kPrecision, 0, true, false};

  NumericLabel hold_label_{2, 0, true, true};
  StatusIcon   status_icon_;

  std::pair<int, int> decomposeTempo(double tempo);

  Gdk::RGBA getBGColor(const Gtk::Widget* widget);
  Gdk::RGBA getFGColor(const Gtk::Widget* widget);

//  void on_style_updated() override;
  void updateCSSClass();
  void onThemeNameChanged();
  void onParentChanged();
};

#endif//GMetronome_TempoDisplay_h
