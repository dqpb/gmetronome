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
  
  void tap(double value = 1.0);
  void reset();
  
  Estimate<double> tempo();
  Estimate<Meter> meter();
  
  double beat();
  
private:

  struct Tap_ 
  {
    std::chrono::time_point<std::chrono::steady_clock> time;
    double value;
  };
  
  struct DataPoint_
  {
    int    time_slot;
    double value;
    double weighted_value;
    double tempo;
  };
  
  std::list<Tap_> taps_;
  std::vector<DataPoint_> corr_;

  void correlate();

  // enum {
  //   kUndefined = -2,
  //   kNoise = -1
  // };
  //void dbscan();
};

#endif//GMetronome_TapAnalyser_h
