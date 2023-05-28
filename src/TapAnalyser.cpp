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
#include <numeric>
#include <glib.h>

#ifndef NDEBUG
# include <iostream>
#endif

namespace {

  using std::chrono::microseconds;
  using std::literals::chrono_literals::operator""us;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""s;

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
  microseconds tap_time {g_get_monotonic_time()};
  double tap_value = std::clamp(value, 0.0, 1.0);
  Flags tap_flags {0x0};

  if (isTimeout(tap_time))
  {
    tap_flags.set(kTimeout);
    reset();
  }
  else if (isOutlier(tap_time))
  {
    tap_flags.set(kOutlier);
    reset();
  }
  else
  {
    tap_flags.set(kValid);
  }

  if (taps_.size() == kMaxTaps)
    taps_.pop_back();

  if (taps_.empty())
    tap_flags.set(kInit);

  taps_.push_front({tap_time, tap_value, tap_flags});

  cached_estimate_ = estimate();

  return {taps_.front(), cached_estimate_};
}

void TapAnalyser::reset()
{
  taps_.clear();
}

bool TapAnalyser::isTimeout(const microseconds& tap_time)
{
  if (taps_.empty())
    return false;

  auto gap = tap_time - taps_.front().time;

  if (gap > kTapTimeout)
    return true;

  return false;
}

bool TapAnalyser::isOutlier(const microseconds& tap_time)
{
  if (taps_.size() >= 2)
  {
    auto gap = tap_time - taps_.front().time;

    if (auto [tempo, phase, confidence] = cached_estimate_; confidence > 0.5)
    {
      minutes_dbl avg_gap {1.0 / tempo};

      if(std::chrono::abs(avg_gap - gap) > 150ms)
        return true;
    }
  }

  return false;
}

TapAnalyser::Estimate TapAnalyser::estimate()
{
  double tempo = 120.0;
  microseconds phase = 0us;
  double confidence = 0.0;

  if (taps_.size() >= 2)
  {
    minutes_dbl sum = std::accumulate(taps_.begin(), taps_.end(), 0us,
                                      [] (microseconds sum, const Tap& tap) {
                                        return sum + tap.time; });

    minutes_dbl avg_time = sum / taps_.size();
    minutes_dbl tap_period = taps_.front().time - taps_.back().time;
    minutes_dbl avg_gap = tap_period / (taps_.size() - 1);

    // compute tempo
    tempo = minutes_dbl(1) / avg_gap;

    // compute the estimated beat position of the last beat;
    // this value is optimal in the sense, that the sum of squared errors (i.e. the
    // deviation of the tappings from the estimated beat positions) is minimized
    phase = std::chrono::duration_cast<microseconds>(avg_time + 0.5 * tap_period);

    // compute confidence
    if (taps_.size() == 2)
    {
      confidence = 0.0;
    }
    else
    {
      // compute standard deviation.
      double sd = 0.0;
      auto first = taps_.begin();
      auto second = std::next(first);
      size_t n = 0;

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
  } // if (taps_.size() >= 2) ...
  else if (taps_.size() == 1)
  {
    phase = taps_.front().time;
  }

  return {tempo, phase, confidence};
}
