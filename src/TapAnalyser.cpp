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

#include "TapAnalyser.h"
#include <algorithm>
#include <cmath>
#include <glib.h>

#include <iostream>

namespace {

  using std::chrono::microseconds;
  using std::literals::chrono_literals::operator""us;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""min;

  const microseconds kTapTimeout = 2s;
  const std::size_t kMaxTaps = 3;

  template<class Iterator>
  microseconds timeGap(const Iterator& it)
  {
    Iterator it2 = it;
    ++it2;
    return it->time - it2->time;
  }

}//unnamed namespace

TapAnalyser::TapAnalyser()
{}

void TapAnalyser::tap(double value)
{
  value = std::clamp(value, 0.0, 1.0);

  if (value == 0.0)
    return;

  microseconds now {g_get_monotonic_time()};

  if (! taps_.empty())
  {
    auto gap = now - taps_.back().time;
    if ( (! taps_.empty() && gap > kTapTimeout)
         || (taps_.size() >= 2 && std::chrono::abs(gap - timeGap( taps_.rbegin())) > 200ms ) )
      reset();
  }

  if (taps_.size() >= kMaxTaps)
    taps_.pop_front();

  taps_.push_back({ now, value });
}

void TapAnalyser::reset()
{
  //std::cout << "RESET ANALYSER." << std::endl;
  taps_.clear();
}

TapAnalyser::Result TapAnalyser::result()
{
  return compute_estimation();
}

TapAnalyser::Result TapAnalyser::compute_estimation()
{
  Result result;

  if (taps_.size() > 1)
  {
    auto gap = timeGap(taps_.rbegin());

    if (gap.count() > 0)
    {
      result.tempo.value = 1min / gap;
      result.tempo.confidence = 1.0;
    }
  }

  return result;
}
