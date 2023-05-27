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
#include "Error.h"
#include <algorithm>
#include <cmath>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace physics {

  namespace {
    using std::chrono::seconds;
    using std::literals::chrono_literals::operator""s;

    // modulo operation that does not trunc (like std::fmod) or round
    // (like std::remainder) the quotient but uses the largest integer
    // value not greater than the result of the division (std::floor)
    constexpr double modulo(double x, double y)
    {
      return x - y * std::floor( x / y );
    }
  }//unnamed namespace


  // SimpleMotion
  SimpleMotion::SimpleMotion(const MotionParameters& params)
    : params_{params}
  {
    // nothing
  }

  void SimpleMotion::set(const MotionParameters& params)
  {
    params_ = params;
  }

  void SimpleMotion::step(const seconds_dbl& time)
  {
    auto& [p,v,a] = params_;
    double v_0 = v;

    v += a * time.count();
    p += 0.5 * (v_0 + v) * time.count();
  }

  // SimpleOscillator
  SimpleOscillator::SimpleOscillator(const MotionParameters& params, double module)
    : SimpleMotion(params),
      m_{module}
  {
    //nothing
  }

  void SimpleOscillator::set(const MotionParameters& params)
  {
    const auto& [p,v,a] = params;
    SimpleMotion::set({modulo(p, m_), v, a});
  }

  void SimpleOscillator::step(const seconds_dbl& time)
  {
    SimpleMotion::step(time);
    SimpleOscillator::set(SimpleMotion::state());
  }

  void SimpleOscillator::remodule(double m)
  {
    assert(m != 0.0);
    m_ = m;
    SimpleOscillator::set(SimpleMotion::state());
  }

  // SimpleSyncOscillator
  SimpleSyncOscillator::SimpleSyncOscillator(const MotionParameters& params, double module)
    : SimpleOscillator(params, module)
  {
    // nothing
  }

  void SimpleSyncOscillator::step(const seconds_dbl& time)
  {
  }

  void SimpleSyncOscillator::syncPosition(double deviation, const seconds_dbl& time)
  {
    // p_m_t1_ = -6.0 * deviation / (time.count() * time.count());
    // p_m_ = m / time.count();
  }

  void SimpleSyncOscillator::syncVelocity(double deviation, const seconds_dbl& time)
  {
    MotionParameters new_params = SimpleMotion::state();
    auto& [p,v,a] = new_params;

    if (isSyncingVelocity()) // remove old sync acceleration
      a -= v_a_;

    if (time == kZeroTime)
    {
      v += deviation;       // instantaneous velocity change
      v_a_ = 0.0;           // and no further acceleration
    }
    else
    {
      v_a_ = deviation / time.count();
      a += v_a_;
    }
    v_time_ = time;
    SimpleOscillator::set(new_params);
  }

  bool SimpleSyncOscillator::isSyncingPosition() const
  {
    return false;
  }

  bool SimpleSyncOscillator::isSyncingVelocity() const
  {
    return v_time_ != kZeroTime;
  }




  /*
  Oscillator::Oscillator(double position,
                         double velocity,
                         double acceleration,
                         double module)
    : p_{position},
      v_{velocity},
      a_{acceleration},
      m_{module},
      a_time_{0.0}
  {
    if (a_ != 0.0)
      a_time_ = kInfiniteTime;
  }

  void Oscillator::accelerate(double a, const seconds_dbl& time)
  {
    a_time_ = (time < kZeroTime) ? kZeroTime : time;

    if (a_time_ == kZeroTime)
      a_ = 0.0;
    else
      a_ = a;
  }

  void Oscillator::step(const seconds_dbl& time)
  {
    if (time <= 0s || time == kInfiniteTime)
      return;

    double v_0 = v_;
    seconds_dbl eff_a_time = std::min(a_time_, time);

    if (eff_a_time > 0s)
    {
      v_ += a_ * eff_a_time.count();
      p_ += 0.5 * (v_0 + v_) * eff_a_time.count();

      a_time_ -= eff_a_time;

      if (a_time_ <= 0s)
        a_ = 0.0;
    }

    p_ += v_ * (time - eff_a_time).count();
    p_ = modulo(p_, m_);
  }

  void Oscillator::remodule(double m)
  {
    assert(m != 0.0);
    m_ = m;
    p_ = modulo(p_, m_);
  }

  void Oscillator::set(double p, double v,
  double a, const seconds_dbl& a_time)
  {
    p_ = modulo(p, m_);
    v_ = v;
    accelerate(a, a_time);
  }

  seconds_dbl Oscillator::arrival(double p) const
  {
    if ( std::isinf(p) )
      return kInfiniteTime;

    p = modulo(p, m_);

    if (p == p_)
      return kZeroTime;

    double p_l = (p < p_) ? p : p - m_;
    double p_r = (p > p_) ? p : p + m_;

    seconds_dbl result {kInfiniteTime};

    if (a_ == 0.0)
    {
      if (v_ > 0.0)
        result = seconds_dbl( (p_r - p_) / v_ );
      else if (v_ < 0.0)
        result = seconds_dbl( (p_l - p_) / v_ );
      else
        result = kInfiniteTime;
    }
    else
    {
      double a = a_ / 2.0;
      double b = v_;
      double c_l = p_ - p_l;
      double c_r = p_ - p_r;

      double b_by_2a = b / (2.0 * a);
      double b_by_2a_squared = b_by_2a * b_by_2a;

      double c_l_by_a = c_l / a;
      double c_r_by_a = c_r / a;

      double radicand_l = b_by_2a_squared - c_l_by_a;
      double radicand_r = b_by_2a_squared - c_r_by_a;

      seconds_dbl t = kInfiniteTime;

      if (radicand_l >= 0.0)
      {
        double root_l = std::sqrt(radicand_l);
        seconds_dbl t_1 {-b_by_2a + root_l};
        seconds_dbl t_2 {-b_by_2a - root_l};

        if (t_1 > kZeroTime && t_1 < t)
          t = t_1;
        if (t_2 > kZeroTime && t_2 < t)
          t = t_2;
      }

      if (radicand_r >= 0.0)
      {
        double root_r = std::sqrt(radicand_r);
        seconds_dbl t_1 {-b_by_2a + root_r};
        seconds_dbl t_2 {-b_by_2a - root_r};

        if (t_1 > kZeroTime && t_1 < t)
          t = t_1;
        if (t_2 > kZeroTime && t_2 < t)
          t = t_2;
      }

      if (t > a_time_) // the target is not reached in the acceleration phase
      {
        double at_dbl = a_time_.count();
        double at_dbl_squared = at_dbl * at_dbl;

        // position after acceleration phase
        double pa = modulo(a * at_dbl_squared + v_ * at_dbl + p_, m_);

        // velocity after acceleration phase
        double va = a_ * at_dbl + v_;

        p_l = (p < pa) ? p : p - m_;
        p_r = (p > pa) ? p : p + m_;

        if (va > 0.0)
          result = a_time_ + seconds_dbl( (p_r - pa) / va );
        else if (va < 0.0)
          result = a_time_ + seconds_dbl( (p_l - pa) / va );
        else
          result = kInfiniteTime;
      }
      else
        result = t;
    }

    return result;
  }
  */
}//namespace physics
