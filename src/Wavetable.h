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

#ifndef GMetronome_Wavetable_h
#define GMetronome_Wavetable_h

#include "Audio.h"
#include "AudioBuffer.h"

#include <vector>
#include <memory>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  class Wavetable {
  public:
    enum class PageResize
    {
      kNoResize,
      kAuto,
      kQuarter,
      kHalf,
      kThreeQuarter
    };

    enum class PageRange : int // cents
    {
      kEqual         = -2,
      kFull          = -1,   // Use page for the full range
      kQuarterOctave = 300,  // Minor third
      kThirdOctave   = 400,  // Major third
      kHalfOctave    = 600,  // Tritone
      kOctave        = 1200,
      kDoubleOctave  = 2400,
    };

  public:
    class Page {
    public:
      using value_type     = float;
      using container_type = std::vector<value_type>;
      using iterator       = container_type::iterator;
      using const_iterator = container_type::const_iterator;

      value_type& operator[](size_t index)
        { return *(begin_+index); }

      const value_type& operator[](size_t index) const
        { return *(begin_+index); }

      // get linear interpolated values
      value_type lookup(float frequency, float time) const
        { return lookup(frequency * time); }

      value_type lookup(float parameter) const
        {
          if (size() == 0)
            return value_type(0);

          [[maybe_unused]] float integral;
          float fractional = std::modf(parameter, &integral);

          if (fractional < 0.0f)
            fractional += 1.0f;

          size_t index1 = fractional * size();
          assert(index1 < size());

          size_t index2 = index1 + 1;

          if (index2 >= size())
            index2 = 0;

          value_type value1 = *(begin_ + index1);
          value_type value2 = *(begin_ + index2);

          return value1 + fractional * (value2 - value1);
        }

      iterator begin()
        { return begin_; }
      const_iterator begin() const
        { return begin_; }
      const_iterator cbegin() const
        { return begin_; }
      iterator end()
        { return end_; }
      const_iterator end() const
        { return end_; }
      const_iterator cend() const
        { return end_; }

      size_t size() const
        { return end_ - begin_; }

    private:
      friend Wavetable;
      iterator begin_;
      iterator end_;
    };

  public:
    using container_type = std::vector<Page>;
    using iterator       = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    Wavetable(size_t n_pages = 0,
              size_t base_page_size = 1024,
              PageResize page_resize = PageResize::kAuto,
              float base_frequency = 40,
              PageRange range = PageRange::kOctave);

    void resize(size_t n_pages = 0,
                size_t base_page_size = 1024,
                PageResize page_resize = PageResize::kAuto,
                float base_frequency = 40,
                PageRange range = PageRange::kOctave);

    PageResize pageResize() const
      { return page_resize_; }

    size_t pageSize(size_t page_index = 0) const
      { return (page_index < size()) ? pages_[page_index].size() : 0; }

    float base(size_t page_index = 0) const;

    PageRange range() const
      { return range_; }

    iterator begin()
      { return pages_.begin(); }
    const_iterator begin() const
      { return pages_.begin(); }
    const_iterator cbegin() const
      { return pages_.begin(); }
    iterator end()
      { return pages_.end(); }
    const_iterator end() const
      { return pages_.end(); }
    const_iterator cend() const
      { return pages_.end(); }

    size_t size() const
      { return pages_.size(); }

    const Page& operator[](size_t page_index) const
      { return pages_[page_index]; }

    Page& operator[](size_t page_index)
      { return pages_[page_index]; }

    const Page& lookup(float frequency) const;

    Page& lookup(float frequency)
      { return const_cast<Page&>(std::as_const(*this).lookup(frequency)); }

    Page::value_type lookup(float frequency, float time) const
      { return lookup(frequency).lookup(frequency,time); }

    void clear();

  private:
    PageResize page_resize_ {PageResize::kHalf};
    float base_ {40.0f};
    PageRange range_ {PageRange::kOctave};
    Page::container_type data_ {};
    container_type pages_ {};
  };

  // class GeneratorBase {
  //   public:
  //     virtual ~GeneratorBase() {}
  //     virtual void prepare(SampleRate rate, float base, PageRange range) {}
  //     virtual value_type operator()(float param) = 0;
  //   };

  // Wavetable(std::shared_ptr<GeneratorBase> generator = nullptr)
  //   : generator_ {generator}
  //   {}

  // void prepare(SampleRate rate  = 44100,
  //              float      base  = 40.0f,
  //              PageRange  range = PageRange::kOctave);

  //   std::shared_ptr<GeneratorBase> generator_;
  // SampleRate rate_ {44100};
  // float base_ {40.0f};
  // PageRange range_ {PageRange::kOctave};
  // std::vector<float> data_ {};
  // std::vector<Page> pages_ {};


}//namespace audio
#endif//GMetronome_Wavetable_h
