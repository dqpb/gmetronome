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
#include <cmath>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace physics {

  namespace {
    constexpr double modulo(double x, double y)
    {
      return x - y * std::floor( x / y );
    }
  }//unnamed namespace

  void ModuloKinematics::accelerate(double a, const seconds_dbl& time)
  {
    a_ = a;
    a_time_ = (time < 0s) ? 0s : time;
  }

  void ModuloKinematics::step(const seconds_dbl& time)
  {
    if (time <= 0s || time == infinite_time)
      return;

    double p_0 = p_;
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

  void ModuloKinematics::remodule(double m)
  {
    //not implemented yet
  }

  void ModuloKinematics::reset(double p, double v, double a)
  {
    //not implemented yet
  }

  seconds_dbl ModuloKinematics::arrival(double p)
  {
    //not implemented yet
    return 0s;
  }

}//namespace physics
