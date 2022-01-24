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

#include <algorithm>
#include <iostream>

#include "Meter.h"

Meter::Meter(int division, int beats, const AccentPattern& accents)
  : division_(division),
    beats_(beats),
    accents_(accents)
{
  checkDataIntegrity();
}

void Meter::setDivision(int division)
{
  division = std::clamp(division, 1, kMaxDivision);

  if (division != division_)
  {
    AccentPattern new_accents(beats_ * division, kAccentOff);

    for (int beat_index = 0; beat_index < beats_; ++beat_index)
    {
      auto source_it = accents_.cbegin() + (beat_index * division_);
      auto target_it = new_accents.begin() + (beat_index * division);
      std::copy_n(source_it, std::min(division, division_), target_it);
    }
    accents_.swap(new_accents);
    division_ = division;
  }
}

void Meter::setBeats(int beats)
{
  beats = std::clamp(beats, 1, kMaxBeats);

  if (beats != beats_)
  {
    AccentPattern new_accents(beats * division_, kAccentOff);

    for (int beat_index = 0; beat_index < beats; ++beat_index)
    {
      if (beat_index < beats_) {
        auto source_it = accents_.cbegin() + (beat_index * division_);
        auto target_it = new_accents.begin() + (beat_index * division_);
        std::copy_n(source_it, division_, target_it);
      }
      else {
        new_accents[beat_index * division_] = kAccentMid;
      }
    }
    accents_.swap(new_accents);
    beats_ = beats;
  }
}

void Meter::setAccentPattern(AccentPattern accents)
{
  accents_ = std::move(accents);
  checkDataIntegrity();
}

void Meter::setAccent(std::size_t index, Accent accent)
{
  if ( index < accents_.size() )
    accents_[index] = accent;
}

void Meter::checkDataIntegrity()
{
  beats_ = std::clamp(beats_, 1, kMaxBeats);
  division_ = std::clamp(division_, 1, kMaxDivision);
  accents_.resize(beats_ * division_, kAccentOff);
  for (auto& a : accents_)
    a = std::clamp(a, kAccentOff, kAccentStrong);
}
