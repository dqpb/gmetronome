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

#ifndef GMetronome_Physics_h
#define GMetronome_Physics_h

#include <chrono>
#include <limits>

namespace physics {

  using seconds_dbl = std::chrono::duration<double>;

  constexpr seconds_dbl kInfiniteTime {std::numeric_limits<double>::infinity()};
  constexpr seconds_dbl kZeroTime {seconds_dbl::zero()};

  class ModuloKinematics {

  public:

    double position() const
      { return p_; }
    double velocity() const
      { return v_; }
    double acceleration() const
      { return a_; }
    double modulus() const
      { return m_; }

    void accelerate(double a)
      { accelerate(a, kInfiniteTime); }

    void accelerate(double a, const seconds_dbl& time);

    void step(const seconds_dbl& time);

    void remodule(double m);

    void reset(double p = 0.0,
               double v = 0.0,
               double a = 0.0,
               const seconds_dbl& a_time = kZeroTime);

    seconds_dbl arrival(double p);

  private:
    double p_{0.0};
    double v_{0.0};
    double a_{0.0};
    double m_{1.0};

    seconds_dbl a_time_{0.0};
  };

}//namespace physics
#endif//GMetronome_Physics_h
