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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "TapAnalyser.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glib.h>

#ifndef NDEBUG
# include <iostream>
#endif

namespace {

  using std::chrono::microseconds;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""min;

  using seconds_dbl = std::chrono::duration<double>;
  using minutes_dbl = std::chrono::duration<double, std::ratio<60>>;

  const microseconds kTapTimeout = 2400ms; // == 25 bpm
  const std::size_t kMaxTaps = 7;

}//unnamed namespace

TapAnalyser::TapAnalyser()
{
  // nothing to do
}

TapAnalyser::Result TapAnalyser::tap(double value)
{
  Flags flags {0x0};

  Tap new_tap {microseconds(g_get_monotonic_time()), std::clamp(value, 0.0, 1.0)};

  if (isTimeout(new_tap))
  {
    flags.set(kTimeout);
    reset();
  }
  else if (isOutlier(new_tap))
  {
    flags.set(kOutlier);
    reset();
  }
  else
  {
    flags.set(kValid);
  }

  if (taps_.size() == kMaxTaps)
    taps_.pop_back();

  taps_.push_front(std::move(new_tap));

  if (taps_.size() == 1)
    flags.set(kInit);

  auto [tempo, confidence] = estimate();

  cached_result_ = std::make_tuple(flags, tempo, confidence);

  return cached_result_;
}

void TapAnalyser::reset()
{
  taps_.clear();
}

bool TapAnalyser::isTimeout(const Tap& tap)
{
  if (taps_.empty())
    return false;

  auto gap = tap.time - taps_.front().time;

  if (gap > kTapTimeout)
    return true;

  return false;
}

bool TapAnalyser::isOutlier(const Tap& tap)
{
  if (taps_.size() >= 2)
  {
    auto gap = tap.time - taps_.front().time;

    if (auto [validity, tempo, confidence] = cached_result_; confidence > 0.5)
    {
      minutes_dbl avg_gap {1.0 / tempo};

      if(std::chrono::abs(avg_gap - gap) > 150ms)
        return true;
    }
  }

  return false;
}

std::tuple<double,double> TapAnalyser::estimate()
{
  double tempo = 120.0;
  double confidence = 0.0;
  minutes_dbl sum = 0min;
  size_t n = 0;

  if (taps_.size() >= 2)
  {
    auto first = taps_.begin();
    auto second = std::next(first);

    for (;second != taps_.end(); ++first, ++second)
    {
      sum += (first->time - second->time);
      ++n;
    }

    minutes_dbl avg_duration = sum / n;

    tempo = minutes_dbl(1) / avg_duration;

    if (taps_.size() == 2)
    {
      confidence = 0.0;
    }
    else
    {
      // compute standard deviation.
      double sd = 0.0;
      first = taps_.begin();
      second = std::next(first);
      n = 0;

      for (;second != taps_.end(); ++first, ++second)
      {
        double dev = (minutes_dbl(1) / (first->time - second->time)) - tempo;
        sd += dev * dev;
        ++n;
      }

      sd = std::sqrt(sd / n);

      // compute the coefficient of variation (CV)
      double cv =  sd / tempo;

      // Use Vangel's approximation to get the confidence interval for CV:
      // Vangel, Mark G. “Confidence Intervals for a Normal Coefficient of Variation.”
      // The American Statistician 50, no. 1 (1996): 21–26. https://doi.org/10.2307/2685039.

      // Percentiles of the chi-squared distribution with significance level alpha=0.05
      // static const std::array<double, 16> kChiSquared1 = {
      //   0,
      //   5.02,
      //   7.38,
      //   9.35,
      //   11.14,
      //   12.83,
      //   14.45,
      //   16.01,
      //   17.53,
      //   19.02,
      //   20.48,
      //   21.92,
      //   23.34,
      //   24.74,
      //   26.12,
      //   27.49
      // };

      static const std::array<double, 16> kChiSquared2 = {
        0,
        0.00098,
        0.0506,
        0.216,
        0.484,
        0.831,
        1.24,
        1.69,
        2.18,
        2.70,
        3.25,
        3.82,
        4.40,
        5.01,
        5.63,
        6.26
      };

      // the approximation is only valid for CV's less than 0.33
      //const double K = std::clamp(cv, 0.0, 0.33);
      const double K = cv;
      const double K_squared = K * K;
      //const double u1 = kChiSquared1[n-1];
      const double u2 = kChiSquared2[n-1];

      // const double rad1 = ((u1 + 2.0) / n - 1.0) * K_squared + u1 / (n - 1);
      // const double ci1 = K / std::sqrt(rad1);

      const double rad2 = ((u2 + 2.0) / n - 1.0) * K_squared + u2 / (n - 1);

      if (rad2 > 0)
      {
        const double ci2 = K / std::sqrt(rad2);

        // scale and map CI to interval [1.0, 0.0]
        confidence = 1.0 - std::tanh(2.0 * ci2);
      }
      else
      {
        confidence = 0.0;
      }
    }
  }

  return std::make_tuple(tempo, confidence);
}
