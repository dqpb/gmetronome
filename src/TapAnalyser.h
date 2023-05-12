/*
 * Copyright (C) 2021-2023 The GMetronome Team
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
#include <tuple>
#include <bitset>
#include <chrono>

class TapAnalyser {
public:

  enum Flag
  {
    kValid = 0,
    kInit = 1,
    kTimeout = 2,
    kOutlier = 3
  };

  using Flags = std::bitset<4>;
  using Result = std::tuple<Flags, double, double>;

public:
  TapAnalyser();

  Result tap(double value = 1.0);

private:

  struct Tap
  {
    std::chrono::microseconds time {0};
    double value {0.0};
  };

  using TapDeque = std::deque<Tap>;
  TapDeque taps_;

  Result cached_result_{"0001", 120.0, 0.0};

  void reset();

  bool isTimeout(const Tap& tap);
  bool isOutlier(const Tap& tap);

  std::tuple<double,double> estimate();
};

#endif//GMetronome_TapAnalyser_h
