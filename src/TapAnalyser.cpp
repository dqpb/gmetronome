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

  using seconds_dbl = std::chrono::duration<double>;
  using minutes_dbl = std::chrono::duration<double, std::ratio<60>>;

  const microseconds kTapTimeout = 2400ms; // == 25 bpm
  const std::size_t kMaxTaps = 8;

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

  Tap new_tap {microseconds(g_get_monotonic_time()), value};

  std::cout << "New tap time: " << new_tap.time.count() << std::endl;

  if (isOutlier(new_tap))
  {
    std::cout << "Tap is outlier." << std::endl;
    reset();
  }
  if (taps_.size() >= kMaxTaps)
    taps_.pop_back();

  taps_.push_front(std::move(new_tap));
}

void TapAnalyser::reset()
{
  std::cout << "Reset Analyser." << std::endl;
  taps_.clear();
}

TapAnalyser::Result TapAnalyser::result()
{
  return computeEstimate();
}

bool TapAnalyser::isOutlier(const Tap& tap)
{
  if (taps_.empty())
    return false;

  auto gap = tap.time - taps_.front().time;

  if (gap > kTapTimeout)
  {
    std::cout << "gap: " << gap.count() << " - Timeout!" << std::endl;
    return true;
  }

  if (taps_.size() >= 2)
  {
    auto first = taps_.begin();
    auto second = std::next(first);

    auto prev_gap = first->time - second->time;

    std::cout << "gap: " << gap.count()
              << " prev gap: " << prev_gap.count()
              << std::endl;

    // This constant is kind of arbitrary but measurements have shown, that humans are
    // pretty good in the synchronization–continuation task (SCT) at 1-3 Hz, so we
    // assume that a deviation slightly larger than 3 Hz means, that we are tapping
    // a different tempo now.
    if(std::chrono::abs(gap - prev_gap) > 150ms)
    {
      std::cout << "Gap outside tolarable variance." << std::endl;
      return true;
    }
  }

  return false;
}

TapAnalyser::Result TapAnalyser::computeEstimate()
{
  Result result;

  minutes_dbl sum = 0min;
  size_t n = 0;

  if (taps_.size() > 1)
  {
    auto first = taps_.begin();
    auto second = std::next(first);

    for (;second != taps_.end(); ++first, ++second)
    {
      sum += (first->time - second->time);
      ++n;
    }

    minutes_dbl avg_duration = sum / n;

    result.tempo.value = minutes_dbl(1) / avg_duration;
    result.tempo.confidence = 1.0;

    std::cout << "taps: " << taps_.size()
              << " gaps: " << n
              << " avg_duration: " << avg_duration.count()
              << " tempo: " << result.tempo.value
              << std::endl;
  }

  return result;
}
