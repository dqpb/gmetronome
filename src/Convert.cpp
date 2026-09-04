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

#include "Convert.h"

std::string boolToString(bool value)
{ return value ? "true" : "false"; }

bool stringToBool(const std::string& text)
{
  if (text == "true")
    return true;
  else if (text == "false")
    return false;
  else if (std::stoi(text) == 0)
    return false;
  else
    return true;
}

#define RAMP_SHAPE_LIST                                         \
  X(audio::EnvelopeRampShape::kLinear,       "linear")          \
  X(audio::EnvelopeRampShape::kCubic,        "cubic")           \
  X(audio::EnvelopeRampShape::kCubicFlipped, "cubic-flipped")

std::string rampShapeToString(audio::EnvelopeRampShape shape)
{
  switch (shape) {
#define X(entry, entry_str) case entry: return entry_str;
    RAMP_SHAPE_LIST
#undef X
  default: return "";
  }
}

audio::EnvelopeRampShape stringToRampShape(const std::string& str)
{
#define X(entry, entry_str) if (str == entry_str) { return entry; }
  RAMP_SHAPE_LIST
#undef X
    return audio::EnvelopeRampShape::kLinear;
}

#define HOLD_SHAPE_LIST                                 \
  X(audio::EnvelopeHoldShape::kKeep,    "keep")         \
  X(audio::EnvelopeHoldShape::kQuartic, "quartic")

std::string holdShapeToString(audio::EnvelopeHoldShape shape)
{
  switch (shape) {
#define X(entry, entry_str) case entry: return entry_str;
    HOLD_SHAPE_LIST
#undef X
  default: return "";
  }
}

audio::EnvelopeHoldShape stringToHoldShape(const std::string& str)
{
#define X(entry, entry_str) if (str == entry_str) { return entry; }
  HOLD_SHAPE_LIST
#undef X
    return audio::EnvelopeHoldShape::kKeep;
}
