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


  Metronome::Metronome()
    : osc_(4.0)
  { }

  void Metronome::start()
  {
    //not implemented yet
  }

  void Metronome::stop()
  {
    //not implemented yet
  }

  void Metronome::setTempo(double tempo)
  {
    tempo_ = bpm_2_bps(tempo);
    osc_.resetVelocity(tempo_);

    if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void Metronome::setTargetTempo(double tempo)
  {
    target_tempo_ = bpm_2_bps(tempo);

    if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void Metronome::setAcceleration(double accel)
  {
    accel_ = bpm2_2_bps2(accel);

    if (force_mode_ == ForceMode::kAccelForce)
      updateOscForce(ForceMode::kAccelForce);
  }

  void Metronome::setMeter(double beats, int division)
  {
    //not implemented yet
  }

  void Metronome::sync(double beat, double tempo, const seconds_dbl& time)
  {
    sync_beat_dev_ = beat;
    sync_tempo_dev_ = bpm_2_bps(tempo);
    sync_start_tempo_ = osc_.velocity();
    sync_time_ = time;

    if (force_mode_ == ForceMode::kSyncForce)
      updateOscForce(ForceMode::kSyncForce);
    else
      switchForceMode(ForceMode::kSyncForce);
  }

  double Metronome::position() const
  { return osc_.position(); }

  double Metronome::tempo() const
  { return bps_2_bpm(osc_.velocity()); }

  double Metronome::acceleration() const
  { return bps2_2_bpm2(osc_.force().base); }


  void Metronome::updateOscForce(ForceMode mode)
  {
    switch (mode) {
    case ForceMode::kNoForce:
    {
      //std::cout << ", noforce";
      osc_.resetForce();
    }
    break;

    case ForceMode::kAccelForce:
    {
      //std::cout << ", accelforce";
      const auto& [force, time] = computeAccelForce(target_tempo_ - osc_.velocity(), accel_);
      accel_r_time_ = time;
      osc_.resetForce(force);
    }
    break;

    case ForceMode::kSyncForce:
    {
      //std::cout << ", syncforce";
      const auto& [force, time] = computeSyncForce(sync_beat_dev_, sync_tempo_dev_, sync_time_);
      sync_r_time_ = time;
      osc_.resetForce(force);
    }
    break;
    };
  }

  void Metronome::switchForceMode(ForceMode mode)
  {
    updateOscForce(mode);
    force_mode_ = mode;
  }

  void Metronome::step(seconds_dbl time)
  {
    if (force_mode_ == ForceMode::kSyncForce)
    {
      seconds_dbl sync_step_time = std::min(sync_r_time_, time);
      osc_.step(sync_step_time);

      time -= sync_step_time;
      sync_r_time_ -= sync_step_time;

      if (sync_r_time_ <= kZeroTime) // handle possible rounding errors
        osc_.resetVelocity(sync_start_tempo_ + sync_tempo_dev_);

      if (time <= kZeroTime)
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
      seconds_dbl accel_step_time = std::min(accel_r_time_, time);
      osc_.step(accel_step_time);

      time -= accel_step_time;
      accel_r_time_ -= accel_step_time;

      if (accel_r_time_ <= kZeroTime) // handle possible rounding errors
        osc_.resetVelocity(target_tempo_);

      if (time <= kZeroTime)
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
        return time;
      }
      else
      {
        applyForce(p0, v0, force, max_time);
        return max_time;
      }
    }

  }//unnamed namespace

  seconds_dbl Metronome::arrival(double p) const
  {
    p = aux::math::modulo(p, osc_.module());

    double v0 = osc_.velocity();
    double p0 = osc_.position();

    seconds_dbl time = kZeroTime;

    if (p == p0)
      return kZeroTime;

    // we assume that the metronome is never going backwards
    if (p < p0)
      p += osc_.module();

    // sync force phase
    if (force_mode_ == ForceMode::kSyncForce)
    {
      const Force& force = osc_.force();
      time += physics::arrival(p0, v0, p, force, sync_r_time_);
    }

    if (p0 >= p)
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

    if (p0 >= p)
      return time;

    // no force phase
    if (v0 != 0.0)
      time += seconds_dbl((p - p0) / v0);
    else
      time = kInfiniteTime;

    return time;
  }

  //static
  std::pair<Force, seconds_dbl>
  Metronome::computeAccelForce(double v_dev, double a)
  {
    std::pair<Force, seconds_dbl> r {{0.0}, kZeroTime};
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

  //static
  std::pair<Force, seconds_dbl>
  Metronome::computeSyncForce(double p_dev, double v_dev, const seconds_dbl& time)
  {
    std::pair<Force, seconds_dbl> r {{0.0}, time};
    auto& [r_force, r_time] = r;

    if (time != kZeroTime)
    {
      double sync_time = time.count();
      double sync_time_squared = sync_time * sync_time;
      double sync_time_cubed = sync_time_squared * sync_time;

      if (p_dev != 0.0)
      {
        r_force.base = 6.0 * p_dev / sync_time_squared;
        r_force.slope = -12.0 * p_dev / sync_time_cubed;
      }

      if (v_dev != 0.0)
        r_force.base += v_dev / sync_time;
    }
    return r;
  }

}//namespace physics
