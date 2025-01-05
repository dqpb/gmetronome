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

  void BeatKinematics::reset()
  {
    tempo_ = 0.0;
    target_ = 0.0;
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
    tempo_ = tempo;

    osc_.resetVelocity(tempo_);

    if (force_mode_ != ForceMode::kNoForce)
      switchForceMode(ForceMode::kNoForce);
  }

  void BeatKinematics::accelerate(double accel, double target)
  {
    accel_ = accel;
    target_ = target;

    if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
    else
      switchForceMode(ForceMode::kAccelForce);
  }

  void BeatKinematics::synchronize(double beat_dev, double tempo_dev, const Time& time)
  {
    sync_beat_dev_ = beat_dev;
    sync_tempo_dev_ = tempo_dev;
    sync_start_tempo_ = osc_.velocity();
    sync_time_ = time;

    if (force_mode_ == ForceMode::kSyncForce)
      updateOscForce(ForceMode::kSyncForce);
    else
      switchForceMode(ForceMode::kSyncForce);
  }

  void BeatKinematics::stopAcceleration()
  {
    if (force_mode_ == ForceMode::kAccelForce)
      switchForceMode(ForceMode::kNoForce);
  }

  void BeatKinematics::stopSynchronization()
  {
    if (force_mode_ == ForceMode::kSyncForce)
      switchForceMode(ForceMode::kNoForce);
  }

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
      const auto& [force, time] = computeAccelForce<Time>(target_ - osc_.velocity(), accel_);
      osc_.resetForce(force, time);
    }
    break;

    case ForceMode::kSyncForce:
    {
      const auto& [force, time] =
        computeSyncForce<Time>(sync_beat_dev_, sync_tempo_dev_, sync_time_);
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

  void BeatKinematics::step(Time time)
  {
    // force phase
    if (force_mode_ != ForceMode::kNoForce)
    {
      time = osc_.step(time);

      // if force time is exceeded handle possible rounding errors
      if (osc_.remainingForceTime() == kZeroTime<Time>)
      {
        if (force_mode_ == ForceMode::kSyncForce)
          osc_.resetVelocity(sync_start_tempo_ + sync_tempo_dev_);
        else if (force_mode_ == ForceMode::kAccelForce)
          osc_.resetVelocity(target_);
      }
      else if (time <= kZeroTime<Time>)
        return;

      switchForceMode(ForceMode::kNoForce);
    }

    // no force phase
    if (time > kZeroTime<Time>)
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
    template<class T>
    T arrival(double& p0, double& v0, double p, const Force<T>& force, const T& max_time)
    {
      double a3 = force.slope / 6.0;
      double a2 = force.base / 2.0;
      double a1 = v0;
      double a0 = p0 - p;

      T time {posmin(aux::math::solveCubic(a3, a2, a1, a0))};

      if (time > kZeroTime<T> && time <= max_time)
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

  BeatKinematics::Time BeatKinematics::arrival(double p_dev) const
  {
    double v0 = osc_.velocity();
    double p0 = osc_.position();
    double p = p0 + p_dev;

    Time time = kZeroTime<Time>;

    if (p == p0)
      return time;

    // force phase
    if (force_mode_ != ForceMode::kNoForce)
    {
      const Force<Time>& force = osc_.force();
      const Time& force_time = osc_.remainingForceTime();
      time += physics::arrival(p0, v0, p, force, force_time);
    }

    if (p0 == p)
      return time;

    // no force phase
    if ((v0 > 0.0 && p > p0) || (v0 < 0.0 && p < p0))
      time += Time((p - p0) / v0);
    else
      time = kInfiniteTime<Time>;

    return time;
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
    while( time > kZeroTime<seconds_dbl> );
  }

}//namespace physics
