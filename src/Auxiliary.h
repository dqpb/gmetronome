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

#ifndef GMetronome_Auxiliary_h
#define GMetronome_Auxiliary_h

#include <cmath>
#include <tuple>
#include <array>

namespace aux {
  namespace math {

    /**
     * @brief     Computes all real solutions of the quadratic equation
     * @return    A tuple containing the number of solutions and the roots
     */
    std::tuple<
      size_t,
      std::array<double,2>
      >
    solveQuadratic(double a2, double a1, double a0);

    /**
     * @brief     Computes all real solutions of the cubic equation
     * @return    A tuple containing the number of solutions and the roots
     */
    std::tuple<
      size_t,
      std::array<double,3>
      >
    solveCubic(double a3, double a2, double a1, double a0);

    /**
     * @brief     Modulo operation that uses the largest integer value
     *            not greater than the result of the division (std::floor)
     */
    template<typename T>
    constexpr std::enable_if_t<std::is_floating_point_v<T>, T> modulo(T a, T b)
    { return a - b * std::floor( a / b ); }

    /**
     * @brief     Overloads for integral types
     */
    template<typename T>
    constexpr std::enable_if_t<std::is_integral_v<T>, T> modulo(T a, T b)
    { return (a % b + b) % b; }

  }//namespace math
}//namespace aux
#endif//GMetronome_Auxiliary_h
