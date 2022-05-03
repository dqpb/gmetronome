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

#include <algorithm>
#include <memory>

namespace audio {

  /**
   * Description of the dimensions and content of a wavetable which is used
   * by the builder to cook a concrete wavetable object.
   * Clients derive from this base class and implement the virtual functions
   * according to the desired properties and content of the wavetable.
   */
  struct WavetableRecipe
  {
    WavetableRecipe()
      { /* nothing */ }
    virtual ~WavetableRecipe()
      { /* nothing */ }
    virtual size_t preferredPages(SampleRate rate) const
      { return 8; };
    virtual size_t preferredBasePageSize(SampleRate rate) const
      { return 8192; }
    virtual Wavetable::PageResize preferredPageResize(SampleRate rate) const
      { return Wavetable::PageResize::kHalf; }
    virtual float preferredBase(SampleRate rate) const
      { return 40.0f; }
    virtual Wavetable::PageRange preferredRange(SampleRate rate) const
      { return Wavetable::PageRange::kOctave; }

    virtual void fillPage(SampleRate rate,
                          size_t page,
                          float base,
                          Wavetable::Page::iterator begin,
                          Wavetable::Page::iterator end) const
      {
        std::fill(begin, end, 0.0f);
      };
  };

  /**
   * Builder class for an ObjectLibrary to create wavetables out of
   * wavetable descriptions. (recipes)
   */
  class WavetableBuilder {
  public:
    WavetableBuilder(SampleRate rate = kDefaultRate);

    void prepare(SampleRate rate);
    Wavetable create(std::shared_ptr<WavetableRecipe> recipe);
    void update(Wavetable& tbl, std::shared_ptr<WavetableRecipe> recipe);

    static Wavetable build(SampleRate rate, const WavetableRecipe& recipe);

  private:
    SampleRate rate_;

    static bool needResize(const Wavetable& tbl, SampleRate rate, const WavetableRecipe& recipe);
    static bool needRebase(const Wavetable& tbl, SampleRate rate, const WavetableRecipe& recipe);
    static void fillTable(Wavetable& tbl, SampleRate rate, const WavetableRecipe& recipe);
  };

  using WavetableLibrary = ObjectLibrary<int, Wavetable, WavetableBuilder>;

}//namespace audio
#endif//GMetronome_WavetableLibrary_h
