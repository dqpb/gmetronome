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

#ifndef GMetronome_TapAnalyser_h
#define GMetronome_TapAnalyser_h

#include "Meter.h"
#include <deque>
#include <chrono>


class TapAnalyser {
public:
  template<typename T>
  struct Estimate
  {
    T value;
    double confidence;
  };

  struct Result
  {
    Estimate<double> tempo {120.0, 0.0};
    Estimate<Meter> meter {kMeter1, 0.0};
    Estimate<double> beat {0.0, 0.0};
  };

public:
  TapAnalyser();

  void tap(double value = 1.0);
  void reset();

  Result result();

private:

  struct Tap
  {
    std::chrono::microseconds time {0};
    double value {0.0};
  };

  using TapDeque = std::deque<Tap>;

  /*
  class Duration
  {
  public:
    TapDeque::const_iterator first_tap();
    TapDeque::const_iterator second_tap();

    std::chrono::microseconds value();

  private:
    TapDeque::const_iterator first_;
    TapDeque::const_iterator second_;
  };

  class DurationView {
  public:
    class Iterator {
    public:

    private:
      const TapDeque& taps_;
      TapDeque::iterator it_;
    };

  public:
    DurationView(const TapDeque& taps) : taps_{taps}
      {}

    Iterator begin();
    Iterator end();

    size_t size() const
      { return taps_.size() / 2; }

  private:
    const TapDeque& taps_;
  };
  */

  TapDeque taps_;

  //DurationView durations_{taps_};

  bool isOutlier(const Tap& tap);
  Result computeEstimate();
};

#endif//GMetronome_TapAnalyser_h
