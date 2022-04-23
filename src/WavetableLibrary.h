/*
 * Copyright (C) 2022 The GMetronome Team
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

#ifndef GMetronome_WavetableLibrary_h
#define GMetronome_WavetableLibrary_h

#include "Audio.h"
#include "Wavetable.h"
#include "ObjectLibrary.h"
#include <functional>

namespace audio {

  class WavetableBuilder {
  public:
    class Repipe
    {
      size_t n_pages {0};
      size_t base_page_size {1024};
      Wavetable::PageResize page_resize {Wavetable::PageResize::kHalf};
      float base_frequency {40};
      Wavetable::PageRange range {Wavetable::PageRange::kOctave};

      std::function<float(SampleRate,float,size_t,)> value_fu;
    };

  public:
    WavetableBuilder(SampleRate rate);

    void prepare(SampleRate rate);
    Wavetable create(const Recipe& recipe);
    void update(Wavetable& table, const Recipe& recipe);

  private:
    SampleRate rate_;
  };

  using WavetableLibrary = ObjectLibrary<int, Wavetable, WavetableBuilder>;

}//namespace audio
#endif//GMetronome_WavetableLibrary_h
