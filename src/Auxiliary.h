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

namespace aux {
  namespace math {

    // modulo operation that uses the largest integer value not greater
    // than the result of the division (std::floor)
    constexpr double modulo(double x, double y)
    { return x - y * std::floor( x / y ); }

  }//namespace math
}//namespace aux
#endif//GMetronome_Auxiliary_h
