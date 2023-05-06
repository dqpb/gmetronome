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
#include <list>
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

  struct Tap_
  {
    std::chrono::microseconds time {0};
    double value {0.0};
  };

  std::list<Tap_> taps_;

  Result compute_estimation();
};

#endif//GMetronome_TapAnalyser_h
