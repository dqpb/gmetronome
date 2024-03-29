/*
 * Copyright (C) 2023 The GMetronome Team
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

#include "Physics.h"
#include <algorithm>
#include <tuple>
#include <array>

#ifndef NDEBUG
# include <iostream>
#endif

namespace physics {

  namespace {

    [[maybe_unused]] constexpr double bpm_2_bps(double bpm)
    { return bpm / 60.0; }

    [[maybe_unused]] constexpr double bps_2_bpm(double bps)
    { return bps * 60.0; }

    [[maybe_unused]] constexpr double bpm2_2_bps2(double bpm2)
    { return bpm2 / 60.0 / 60.0; }

    [[maybe_unused]] constexpr double bps2_2_bpm2(double bps2)
    { return bps2 * 60.0 * 60.0; }

  }//unnamed namespace

  void BeatKinematics::reset()
  {
    tempo_ = 0.0;
    target_tempo_ = 0.0;
    accel_ = 0.0;

    osc_.reset();
    switchForceMode(ForceMode::kNoForce);
  }

  void BeatKinematics::setBeats(double beats, bool turnover)
  {
    osc_.remodule(beats);

    if (turnover)
    {
      double integral;
      double fractional = std::modf(osc_.position(), &integral);

      double new_position = osc_.module() - 1.0 + fractional;

      osc_.resetPosition(new_position);
    }
  }

  void BeatKinematics::setTempo(double tempo)
  {
    tempo_ = bpm_2_bps(tempo);

    osc_.resetVelocity(tempo_);

    if (force_mode_ == ForceMode::kSyncForce)
    {
      if (tempo_ != target_tempo_ && accel_ != 0)
        switchForceMode(ForceMode::kAccelForce);
      else
        switchForceMode(ForceMode::kNoForce);
    }
    else if (force_mode_ == ForceMode::kNoForce && tempo_ != target_tempo_ && accel_ != 0)
      switchForceMode(ForceMode::kAccelForce);
    else if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void BeatKinematics::setTargetTempo(double tempo)
  {
    target_tempo_ = bpm_2_bps(tempo);

    if (force_mode_ == ForceMode::kNoForce && tempo_ != target_tempo_ && accel_ != 0)
      switchForceMode(ForceMode::kAccelForce);
    else if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void BeatKinematics::setAcceleration(double accel)
  {
    accel_ = bpm2_2_bps2(accel);

    if (force_mode_ == ForceMode::kNoForce && tempo_ != target_tempo_ && accel_ != 0)
      switchForceMode(ForceMode::kAccelForce);
    else if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void BeatKinematics::synchronize(double beat_dev, double tempo_dev, const seconds_dbl& time)
  {
    sync_beat_dev_ = beat_dev;
    sync_tempo_dev_ = bpm_2_bps(tempo_dev);
    sync_start_tempo_ = osc_.velocity();
    sync_time_ = time;

    if (force_mode_ == ForceMode::kSyncForce)
      updateOscForce(ForceMode::kSyncForce);
    else
      switchForceMode(ForceMode::kSyncForce);
  }

  double BeatKinematics::position() const
  { return osc_.position(); }

  double BeatKinematics::tempo() const
  { return bps_2_bpm(osc_.velocity()); }

  double BeatKinematics::acceleration() const
  { return bps2_2_bpm2(osc_.force().base); }


  void BeatKinematics::updateOscForce(ForceMode mode)
  {
    switch (mode) {
    case ForceMode::kNoForce:
    {
      osc_.resetForce();
    }
    break;

    case ForceMode::kAccelForce:
    {
      const auto& [force, time] = computeAccelForce(target_tempo_ - osc_.velocity(), accel_);
      osc_.resetForce(force, time);
    }
    break;

    case ForceMode::kSyncForce:
    {
      const auto& [force, time] = computeSyncForce(sync_beat_dev_, sync_tempo_dev_, sync_time_);
      osc_.resetForce(force, time);
    }
    break;
    };
  }

  void BeatKinematics::switchForceMode(ForceMode mode)
  {
    updateOscForce(mode);
    force_mode_ = mode;
  }

  void BeatKinematics::step(seconds_dbl time)
  {
    if (force_mode_ == ForceMode::kSyncForce)
    {
      time = osc_.step(time);

      // if force time is exceeded handle possible rounding errors
      if (osc_.remainingForceTime() == kZeroTime)
        osc_.resetVelocity(sync_start_tempo_ + sync_tempo_dev_);
      else if (time <= kZeroTime)
        return;
    }

    if (force_mode_ != ForceMode::kAccelForce
        && osc_.velocity() != target_tempo_
        && accel_ != 0.0)
    {
      switchForceMode(ForceMode::kAccelForce);
    }

    if (force_mode_ == ForceMode::kAccelForce)
    {
      time = osc_.step(time);

      if (osc_.remainingForceTime() == kZeroTime)
        osc_.resetVelocity(target_tempo_);
      else if (time <= kZeroTime)
        return;
    }

    if (force_mode_ != ForceMode::kNoForce)
      switchForceMode(ForceMode::kNoForce);

    if (time > kZeroTime)
      osc_.step(time);
  }

  namespace {

    // Returns the smallest positive value in the array or a negative
    // value, if no such value exists.
    // This is used in conjunction with the return values of
    // aux::math::solveCubic and aux::math::solveQuadratic.
    template<size_t S>
    double posmin(const std::tuple<size_t, std::array<double,S>>& tpl)
    {
      const auto& [n,a] = tpl;

      if (n == 0)
        return -1;

      double m = std::numeric_limits<double>::infinity();

      for (size_t i = 0; i < n; ++i)
        if (a[i] >= 0.0 && a[i] < m)
          m = a[i];

      if (m == std::numeric_limits<double>::infinity())
        return -1;
      else
        return m;
    }

    // Computes the time until a given position is reached under the influence
    // of a force and updates position and velocity. If the position is never
    // reached or not reached within the time limit max_time, the time limit
    // is returned.
    seconds_dbl arrival(double& p0, double& v0, double p,
                        const Force& force, const seconds_dbl& max_time)
    {
      double a3 = force.slope / 6.0;
      double a2 = force.base / 2.0;
      double a1 = v0;
      double a0 = p0 - p;

      seconds_dbl time {posmin(aux::math::solveCubic(a3, a2, a1, a0))};

      if (time > kZeroTime && time <= max_time)
      {
        applyForce(p0, v0, force, time);
        p0 = p; // handle limited fp precision
        return time;
      }
      else
      {
        applyForce(p0, v0, force, max_time);
        return max_time;
      }
    }

  }//unnamed namespace

  seconds_dbl BeatKinematics::arrival(double p_dev) const
  {
    double v0 = osc_.velocity();
    double p0 = osc_.position();
    double p = p0 + p_dev;

    seconds_dbl time = kZeroTime;

    if (p == p0)
      return time;

    // sync force phase
    if (force_mode_ == ForceMode::kSyncForce)
    {
      const Force& force = osc_.force();
      const seconds_dbl& force_time = osc_.remainingForceTime();
      time += physics::arrival(p0, v0, p, force, force_time);
    }

    if (p0 == p)
      return time;

    // accel force phase
    if (v0 != target_tempo_ && accel_ != 0.0)
    {
      if (double v_dev = target_tempo_ - v0; v_dev != 0.0)
      {
        auto [force, force_time] = computeAccelForce(v_dev, accel_);
        time += physics::arrival(p0, v0, p, force, force_time);
      }
    }

    if (p0 == p)
      return time;

    // no force phase
    if ((v0 > 0.0 && p > p0) || (v0 < 0.0 && p < p0))
      time += seconds_dbl((p - p0) / v0);
    else
      time = kInfiniteTime;

    return time;
  }

  std::pair<Force, seconds_dbl>
  computeAccelForce(double v_dev, double a)
  {
    std::pair<Force, seconds_dbl> r {{0.0, 0.0}, kZeroTime};
    auto& [r_force, r_time] = r;

    if (v_dev != 0.0)
    {
      if (a == 0.0)
      {
        r_time = kInfiniteTime;
        r_force = {0.0, 0.0};
      }
      else if (seconds_dbl t { v_dev / a }; t > kZeroTime)
      {
        r_time = t;
        r_force = {a, 0.0};
      }
      else
      {
        r_time = -t;
        r_force = {-a, 0.0};
      }
    }
    else
    {
      r_time = kZeroTime;
      r_force = {a, 0.0};
    }
    return r;
  }

  std::pair<Force, seconds_dbl>
  computeAccelForce(double v_dev, const seconds_dbl& time)
  {
    std::pair<Force, seconds_dbl> r {{0.0, 0.0}, time};
    auto& [r_force, r_time] = r;

    if (time != kZeroTime)
    {
      r_force.base = v_dev / time.count();
    }
    return r;
  }

  std::pair<Force, seconds_dbl>
  computeSyncForce(double p_dev, double v_dev, const seconds_dbl& time)
  {
    std::pair<Force, seconds_dbl> r {{0.0, 0.0}, time};
    auto& [r_force, r_time] = r;

    if (time != kZeroTime)
    {
      double sync_time = time.count();
      double sync_time_squared = sync_time * sync_time;
      double sync_time_cubed = sync_time_squared * sync_time;

      if (p_dev != 0.0)
      {
        r_force.base += 6.0 * p_dev / sync_time_squared;
        r_force.slope += -12.0 * p_dev / sync_time_cubed;
      }

      if (v_dev != 0.0)
      {
        r_force.base += -2.0 * v_dev / sync_time;
        r_force.slope += 6.0 * v_dev / sync_time_squared;
      }
    }
    return r;
  }


  void PendulumKinematics::shutdown(const seconds_dbl& time)
  {
    osc_.resetForce(computeAccelForce(-osc_.velocity(), time));
  }

  void PendulumKinematics::synchronize(double theta_dev, double omega_dev, const seconds_dbl& time)
  {
    const auto& [force, force_time] = computeSyncForce(theta_dev, omega_dev, time);
    osc_.resetForce(force, force_time);
  }

  void PendulumKinematics::step(seconds_dbl time)
  {
    do {
      time = osc_.step(time);
    }
    while( time > kZeroTime );
  }

}//namespace physics
