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
  SimpleMotion::SimpleMotion(double position,
                             double velocity,
                             double acceleration)
    : p_{position},
      v_{velocity},
      a_{acceleration}
  {
    // nothing
  }

  void SimpleMotion::accelerate(double a)
  {
    a_ = a;
  }

  void SimpleMotion::reset(double p, double v, double a)
  {
    p_ = p;
    v_ = v;
    SimpleMotion::accelerate(a);
  }

  void SimpleMotion::step(const seconds_dbl& time)
  {
    std::tie(p_,v_) = step(p_,v_,a_,time);
  }

  // SimpleOscillator
  SimpleOscillator::SimpleOscillator(double position,
                                     double velocity,
                                     double acceleration,
                                     double module)
    : SimpleMotion(position, velocity, acceleration),
      m_{module}
  {
    //nothing
  }

  void SimpleOscillator::reset(double p, double v, double a)
  {
    SimpleMotion::reset(modulo(p, m_), v, a);
  }

  void SimpleOscillator::step(const seconds_dbl& time)
  {
    SimpleMotion::step(time);
    SimpleMotion::reset(modulo(position(), m_), velocity(), acceleration());
  }

  void SimpleOscillator::remodule(double m)
  {
    assert(m != 0.0);
    m_ = m;
    SimpleMotion::reset(modulo(position(), m_), velocity(), acceleration());
  }

  // SimpleSyncOscillator
  SimpleSyncOscillator::SimpleSyncOscillator(double position,
                                             double velocity,
                                             double acceleration,
                                             double module)
    : SimpleOscillator(position, velocity, acceleration, module)
  {
    // nothing
  }

  void SimpleSyncOscillator::syncPosition(double deviation)
  {
    p_dev_ = deviation;
  }

  void SimpleSyncOscillator::syncVelocity(double deviation)
  {
    v_dev_ = deviation;
  }

  void SimpleSyncOscillator::accelerate(double a)
  {
    SimpleOscillator::accelerate(a);
    syncPosition(0.0);
    syncVelocity(0.0);
  }

  void SimpleSyncOscillator::step(const seconds_dbl& time)
  {
    static constexpr double kMaxAcceleration = 5.0;

    double a = kMaxAcceleration * std::tanh(p_dev_ + v_dev_);

    if (a != 0.0)
    {
      auto [p,v] = SimpleMotion::step(position(), velocity(), a, time);

      p_dev_ -= (p_dev_ != 0.0) ? p - position() : 0.0;
      v_dev_ -= (v_dev_ != 0.0) ? v - velocity() : 0.0;

      SimpleOscillator::accelerate(a);
    }
    SimpleOscillator::step(time);
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

  void Oscillator::reset(double p, double v,
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
