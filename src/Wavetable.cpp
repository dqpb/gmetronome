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
    resize(n_pages, base_page_size, page_resize);
    rebase(base_frequency, range);
  }

  void Wavetable::resize(size_t n_pages,
                         size_t base_page_size,
                         PageResize page_resize)
  {
    if (n_pages == 0)
    {
      pages_.clear();
      data_.clear();
      return;
    }

    size_t data_size = 0;

    switch(page_resize) {
    case PageResize::kNoResize:
      data_size = n_pages * base_page_size;
      break;
    case PageResize::kThreeQuarter:
      // compute the first n_pages terms of the geometric series with r=3/4 and a=base_page_size
      data_size = std::floor(base_page_size * (1.0f - std::pow(0.75f, n_pages)) / (1.0f - 0.75f));
      break;
    case PageResize::kHalf:
       data_size = std::floor(base_page_size * (1.0f - std::pow(0.5f, n_pages)) / (1.0f - 0.5f));
      break;
    case PageResize::kQuarter:
       data_size = std::floor(base_page_size * (1.0f - std::pow(0.25f, n_pages)) / (1.0f - 0.25f));
      break;
    };

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

      switch(page_resize) {
      case PageResize::kNoResize: break;
      case PageResize::kThreeQuarter: page_increment *= 3.0 / 4.0; break;
      case PageResize::kHalf: page_increment /= 2; break;
      case PageResize::kQuarter: page_increment /= 4; break;
      };
    }
  }

  void Wavetable::rebase(float base, PageRange range)
  {
    assert(base > 0.0f);
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
    case PageRange::kHalfOctave:
      page_base = base_ * std::pow(3.0 / 2.0, page_index);
      break;
    case PageRange::kOctave:
      page_base = base_ * std::pow(2, page_index);
      break;
    case PageRange::kDoubleOctave:
      page_base = base_ * std::pow(4, page_index);
      break;
    case PageRange::kFull:
      page_base = base_;
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
    case PageRange::kHalfOctave:
      preferred_page_index
        = std::floor( (std::log(frequency) - std::log(base_)) / std::log(3.0f / 2.0f));
      break;
    case PageRange::kOctave:
      preferred_page_index = std::floor(std::log2(frequency) - std::log2(base_));
      break;
    case PageRange::kDoubleOctave:
      preferred_page_index
        = std::floor( (std::log(frequency) - std::log(base_)) / std::log(4.0f));
      break;
    case PageRange::kFull:
      preferred_page_index = 0;
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
