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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Wavetable.h"
#include "Error.h"

#include <cassert>
#include <cmath>
#include <utility>

namespace audio {

  Wavetable::Wavetable(size_t n_pages,
                       size_t base_page_size,
                       PageResize page_resize,
                       float base_frequency,
                       PageRange range)
  {
    resize(n_pages, base_page_size, page_resize, base_frequency, range);
  }

  void Wavetable::resize(size_t n_pages,
                         size_t base_page_size,
                         PageResize page_resize,
                         float base,
                         PageRange range)
  {
    assert(base > 0.0f);

    if (n_pages == 0)
    {
      pages_.clear();
      data_.clear();
      return;
    }

    float resize_factor = 1.0f;

    switch(page_resize) {
    case PageResize::kNoResize:
      resize_factor = 1.0f;
      break;
    case PageResize::kAuto:
      switch (range) {
      case PageRange::kEqual:         resize_factor = 1.0f; break;
      case PageRange::kFull:          resize_factor = 1.0f; break;
      case PageRange::kQuarterOctave: resize_factor = 1.0f / std::pow(2.0f, 1.0f / 4.0f); break;
      case PageRange::kThirdOctave:   resize_factor = 1.0f / std::pow(2.0f, 1.0f / 3.0f); break;
      case PageRange::kHalfOctave:    resize_factor = 1.0f / std::pow(2.0f, 1.0f / 2.0f); break;
      case PageRange::kOctave:        resize_factor = 1.0f / std::pow(2.0f, 1.0f); break;
      case PageRange::kDoubleOctave:  resize_factor = 1.0f / std::pow(2.0f, 2.0f); break;
      };
      break;
    case PageResize::kThreeQuarter:
      resize_factor = 3.0f / 4.0f;
      break;
    case PageResize::kHalf:
      resize_factor = 1.0f / 2.0f;
      break;
    case PageResize::kQuarter:
      resize_factor = 1.0f / 4.0f;
      break;
    };

    size_t data_size = 0;
    if (resize_factor == 1.0f)
    {
      data_size = n_pages * base_page_size;
    }
    else
    {
      // compute the first n_pages terms of the geometric series with r=resize_factor
      // and a=base_page_size
      data_size = std::floor(
        base_page_size * (1.0f - std::pow(resize_factor, n_pages)) / (1.0f - resize_factor));
    }

    data_.resize(data_size);

    // set up pages
    pages_.resize(n_pages);

    auto it = data_.begin();
    size_t page_increment = base_page_size;
    for (auto& page : pages_)
    {
      page.begin_ = it;
      it += page_increment;
      page.end_ = it;
      page_increment *= resize_factor;
    }

    page_resize_ = page_resize;
    base_ = base;
    range_ = range;
  }

  float Wavetable::base(size_t page_index) const
  {
    if (page_index == 0)
      return base_;

    float page_base = 0.0f;

    switch(range_) {
    case PageRange::kEqual:
      page_base = base_ * (page_index + 1);
      break;
    case PageRange::kFull:
      page_base = base_;
      break;
    case PageRange::kQuarterOctave:
      page_base = base_ * std::pow( std::pow(2.0, 1.0 / 4.0), page_index);
      break;
    case PageRange::kThirdOctave:
      page_base = base_ * std::pow( std::pow(2.0, 1.0 / 3.0), page_index);
      break;
    case PageRange::kHalfOctave:
      page_base = base_ * std::pow( std::pow(2.0, 1.0 / 2.0), page_index);
      break;
    case PageRange::kOctave:
      page_base = base_ * std::pow( std::pow(2.0, 1.0), page_index);
      break;
    case PageRange::kDoubleOctave:
      page_base = base_ * std::pow( std::pow(2.0, 2.0), page_index);
      break;
    };

    return page_base;
  }

  const Wavetable::Page& Wavetable::lookup(float frequency) const
  {
    assert(frequency > 0.0f);

    if (pages_.empty())
      throw GMetronomeError {"page not found"};

    if (pages_.size() == 1 || frequency < base_)
      return pages_.front();

    size_t preferred_page_index = 0;

    switch(range_) {
    case PageRange::kEqual:
      preferred_page_index = std::floor(frequency / base_);
      break;
    case PageRange::kFull:
      preferred_page_index = 0;
      break;
    case PageRange::kQuarterOctave:
      preferred_page_index = std::floor( std::log2(frequency / base_) / (1.0 / 4.0) );
      break;
    case PageRange::kThirdOctave:
      preferred_page_index = std::floor( std::log2(frequency / base_) / (1.0 / 3.0) );
      break;
    case PageRange::kHalfOctave:
      preferred_page_index = std::floor( std::log2(frequency / base_) / (1.0 / 2.0) );
      break;
    case PageRange::kOctave:
      preferred_page_index = std::floor( std::log2(frequency / base_) / 1.0 );
      break;
    case PageRange::kDoubleOctave:
      preferred_page_index = std::floor( std::log2(frequency / base_) / 2.0 );
      break;
    };

    return pages_[std::min(preferred_page_index, pages_.size() - 1)];
  }

  void Wavetable::clear()
  {
    pages_.clear();
    data_.clear();
  }

}//namespace audio
