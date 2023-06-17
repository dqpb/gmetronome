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

#include "Auxiliary.h"

#ifndef NDEBUG
# include <iostream>
#endif

namespace aux {
  namespace math {
    namespace {

      // solve cubic with one real solution
      double solveCubic1(double a3, double a2, double a1, double a0,
                         double q, double r, double r2_plus_q3)
      {
        double A = std::cbrt(std::abs(r) + std::sqrt(r2_plus_q3));
        double t1 = (r >= 0) ? A - q / A : q / A - A;
        return t1 - a2 / 3.0;
      }

      // solve cubic with three real solutions
      std::tuple<
        size_t,
        std::array<double,3>
        >
      solveCubic3(double a3, double a2, double a1, double a0,
                  double q, double r)
      {
        double minus_q = -q;
        double minus_q3 = minus_q * minus_q * minus_q;

        double T = (q == 0) ? 0.0 : std::acos(r / std::sqrt(minus_q3));
        double p1 = T / 3.0;
        double p2 = p1 - 2 * M_PI / 3.0;
        double p3 = p1 + 2 * M_PI / 3.0;

        double two_sqrt_minus_q = 2.0 * std::sqrt(minus_q);
        double a2_by_3 = a2 / 3.0;

        return { 3, {
            two_sqrt_minus_q * std::cos(p1) - a2_by_3,
            two_sqrt_minus_q * std::cos(p2) - a2_by_3,
            two_sqrt_minus_q * std::cos(p3) - a2_by_3
          }
        };
      }

    }//unnamed namespace

    std::tuple<
      size_t,
      std::array<double,2>
      >
    solveQuadratic(double a2, double a1, double a0)
    {
      double radicand = a1 * a1 - 4.0 * a2 *a0;

      if (radicand < 0.0)
        return {0, {0.0, 0.0}};

      double A = std::sqrt(radicand);

      // use different formulas to provide numerically stable solutions and
      // prevent subtractive cancellation
      if (a1 >= 0.0)
        return {2, {(-a1 - A) / (2.0 * a2), (2.0 * a0) / (-a1 - A)}};
      else
        return {2, {(2.0 * a0) / (-a1 + A), (-a1 + A) / (2.0 * a2)}};
    }

    std::tuple<
      size_t,
      std::array<double,3>
      >
    solveCubic(double a3, double a2, double a1, double a0)
    {
      if (a3 == 0.0)
      {
        auto [n,r] = solveQuadratic(a2, a1, a0);
        return {n, {r[0], r[1], 0.0}};
      }
      else
      {
        a0 /= a3;
        a1 /= a3;
        a2 /= a3;
        a3 = 1.0;
      }
      double q = a1 / 3.0 - (a2 * a2) / 9.0;
      double r = (a1 * a2 - 3.0 * a0) / 6.0 - (a2 * a2 * a2) / 27.0;
      double r2_plus_q3 = r * r + q * q * q;

      if (r2_plus_q3 > 0.0)
        return {1, {solveCubic1(a3,a2,a1,a0,q,r,r2_plus_q3), 0.0, 0.0}};
      else
        return solveCubic3(a3,a2,a1,a0,q,r);
    }

  }//namespace math
}//namespace aux
