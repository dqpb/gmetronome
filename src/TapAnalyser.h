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
    kInit     = 0,
    kValid    = 1,
    kTimeout  = 2,
    kOutlier  = 3,
    kNumFlags
  };

  using Flags = std::bitset<kNumFlags>;

  struct Tap
  {
    std::chrono::microseconds time {0};
    double value {0.0};
    Flags flags {0x0};
  };

  struct Estimate
  {
    double tempo {120.0};                  // estimated tempo
    std::chrono::microseconds phase {0};   // estimated phase
    double confidence {0.0};               // confidence
  };

  using Result = std::tuple<const Tap&, const Estimate&>;

public:
  TapAnalyser();

  Result tap(double value = 1.0);

private:
  using TapDeque = std::deque<Tap>;
  TapDeque taps_;

  void reset();

  bool isTimeout(const std::chrono::microseconds& tap_time);
  bool isOutlier(const std::chrono::microseconds& tap_time);

  Estimate cached_estimate_;

  Estimate estimate();
};

#endif//GMetronome_TapAnalyser_h
