/*
 * Copyright (C) 2021 The GMetronome Team
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

#ifndef GMetronome_Error_h
#define GMetronome_Error_h

#include <string>

class GMetronomeError : public std::exception {
public:
  GMetronomeError(const std::string& message = "");
  
  const char* what() const noexcept
    { return message_.data(); }
  
private:
  const std::string message_;
};

const GMetronomeError kGenericError {"An unspecified error occured."};

#endif//GMetronome_Error_h
