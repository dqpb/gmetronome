/*
 * Copyright (C) 2026 The GMetronome Team
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

#ifndef GMetronome_Convert_h
#define GMetronome_Convert_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Synthesizer.h"

#include <string>
#include <stdexcept>
#include <cmath>
#ifdef HAVE_CPP_LIB_TO_CHARS
# include <array>
# include <charconv>
#else
# include <sstream>
#endif

// Convert number to strin
template<class T>
std::string numberToString(const T& value)
{
#ifdef HAVE_CPP_LIB_TO_CHARS
  constexpr int kConvBufSize = 50;
  std::array<char,kConvBufSize> str;
  if(auto [p, ec] = std::to_chars(str.data(), str.data() + str.size(), value);
     ec == std::errc())
    return std::string(str.data(), p - str.data());
  else
    throw std::runtime_error {"failed to convert number to string"};
#else
  std::stringstream sstr;
  sstr.imbue(std::locale::classic());
  sstr << value;

  std::string s;
  sstr >> s;

  if (sstr.fail())
    throw std::runtime_error {"failed to convert number to string"};

  return s;
#endif
}

// Convert string to number
template<class T>
T stringToNumber(const std::string& str)
{
#ifdef HAVE_CPP_LIB_TO_CHARS
  T value;
  if(auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
     ec == std::errc())
    return value;
  else
    throw std::runtime_error {"failed to convert string to number"};
#else
  std::stringstream sstr;
  sstr.imbue(std::locale::classic());
  sstr << str;

  T value;
  sstr >> value;

  if (sstr.fail())
    throw std::runtime_error {"failed to convert string to number"};

  return value;
#endif
}

// Convert double to string with precision 2
inline std::string doubleToString(double value)
{ return numberToString(std::round(value * 100.0) / 100.0); }

// Convert string to double with precision 2
inline double stringToDouble(const std::string& str)
{ return std::round(stringToNumber<double>(str) * 100.0) / 100.0; }

// Convert int to string
inline std::string intToString(int value)
{ return numberToString(value); }

// Convert string to int
inline int stringToInt(const std::string& str)
{ return stringToNumber<int>(str); }

// Convert bool to string
std::string boolToString(bool value);

// Convert string to bool
bool stringToBool(const std::string& text);

// Convert audio::EnvelopeRampShape to string
std::string rampShapeToString(audio::EnvelopeRampShape shape);

// Convert string to audio::EnvelopeRampShape
audio::EnvelopeRampShape stringToRampShape(const std::string& str);

// Convert audio::EnvelopeHoldShape to string
std::string holdShapeToString(audio::EnvelopeHoldShape shape);

// Convert string to audio::EnvelopeHoldShape
audio::EnvelopeHoldShape stringToHoldShape(const std::string& str);

inline std::string decibelToString(const audio::Decibel& d)
{ return doubleToString(d.value()); }

inline audio::Decibel stringToDecibel(const std::string& str)
{ return audio::Decibel(stringToDouble(str)); }

#endif//GMetronome_Convert_h
