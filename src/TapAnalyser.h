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
#include <vector>
#include <chrono>

template<typename T>
struct Estimate
{
  T value;
  double confidence;
};

class TapAnalyser {
public:
  TapAnalyser();
  
  void tap();
  void reset();
  
  Estimate<double> tempo();
  Estimate<Meter> meter();
  
  double beat();
  
private:
  std::chrono::time_point<std::chrono::steady_clock> tap_start_;
  struct Tap_
  {
    int slot;
    double value;
  };
  std::vector<Tap_> taps_;
  std::vector<Tap_> corr_;
  
  void correlate();
};

#endif//GMetronome_TapAnalyser_h
