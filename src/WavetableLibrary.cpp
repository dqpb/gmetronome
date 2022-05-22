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

#include "WavetableLibrary.h"
#include <cmath>
#include <cassert>

namespace audio {

  void SineRecipe::fillPage(SampleRate rate,
                            size_t page,
                            float base,
                            Wavetable::Page::iterator begin,
                            Wavetable::Page::iterator end) const
  {
    size_t page_size = end - begin;
    double step = 2.0 * M_PI / page_size;
    for (size_t index = 0; index < page_size; ++index, ++begin)
      *begin = std::sin(index * step);
  }

  void TriangleRecipe::fillPage(SampleRate rate,
                                size_t page,
                                float base,
                                Wavetable::Page::iterator begin,
                                Wavetable::Page::iterator end) const
  {
    size_t page_size = end - begin;

    // Since we requested a range of one (one half) octave per page (see TriangleRecipe)
    // the highest fundamental in this page is 2^1 (2^0.5) * base. We use this fundamental
    // as starting point to compute the highest possible harmonic that we can use to compute
    // the triangle waveform.
    float fundamental = std::pow(2.0f, 1.0f /*1.0f / 2.0f*/) * base;

    // By Nyquist we must not produce higher frequencies than half the sample rate.
    size_t max_harmonic_1 = (rate / 2.0) / fundamental;

    // We are also limited by the actual page size as we need twice the number of wavetable
    // samples for a maximum harmonic. (A given fundamental for the wavetable also gives a
    // sample rate for the table which again limits the representable frequencies by Nyquist).
    size_t max_harmonic_2 = page_size / 2;

    // We use the minimum of these bounds:
    int max_harmonic = std::min(max_harmonic_1, max_harmonic_2);

    double step = 2.0 * M_PI / page_size;

    for (size_t index = 0; index < page_size; ++index, ++begin)
    {
      double sum = 0;
      for (int harmonic = 1, sign = 1; harmonic <= max_harmonic; harmonic += 2, sign *= -1)
        sum += sign * 1.0 / (harmonic * harmonic) * std::sin(harmonic * index * step);

      *begin = 8.0 / (M_PI * M_PI) * sum;
    }
  }

  void SawtoothRecipe::fillPage(SampleRate rate,
                                size_t page,
                                float base,
                                Wavetable::Page::iterator begin,
                                Wavetable::Page::iterator end) const
  {
    size_t page_size = end - begin;
    float fundamental = std::pow(2.0f, 1.0f /*1.0f / 2.0f*/) * base;
    size_t max_harmonic_1 = (rate / 2.0) / fundamental;
    size_t max_harmonic_2 = page_size / 2;
    int max_harmonic = std::min(max_harmonic_1, max_harmonic_2);
    double step = 2.0 * M_PI / page_size;

    for (size_t index = 0; index < page_size; ++index, ++begin)
    {
      double sum = 0;
      for (int harmonic = 1, sign = -1; harmonic <= max_harmonic; ++harmonic, sign *= -1)
        sum += sign * 1.0 / harmonic * std::sin(harmonic * index * step);

      *begin = 2.0 / M_PI * sum;
    }
  }

  void SquareRecipe::fillPage(SampleRate rate,
                              size_t page,
                              float base,
                              Wavetable::Page::iterator begin,
                              Wavetable::Page::iterator end) const
  {
    size_t page_size = end - begin;
    float fundamental = std::pow(2.0f, 1.0f /* 1.0f / 2.0f */) * base;
    size_t max_harmonic_1 = (rate / 2.0) / fundamental;
    size_t max_harmonic_2 = page_size / 2;
    int max_harmonic = std::min(max_harmonic_1, max_harmonic_2);
    double step = 2.0 * M_PI / page_size;

    for (size_t index = 0; index < page_size; ++index, ++begin)
    {
      double sum = 0;
      for (int harmonic = 1; harmonic <= max_harmonic; harmonic += 2)
        sum += 1.0 / harmonic * std::sin(harmonic * index * step);

      *begin = 4.0 / M_PI * sum;
    }
  }

  WavetableBuilder::WavetableBuilder(SampleRate rate) : rate_{rate}
  { /* nothing */ }

  void WavetableBuilder::prepare(SampleRate rate)
  { rate_ = rate; }

  Wavetable WavetableBuilder::create(std::shared_ptr<WavetableRecipe> recipe)
  {
    assert(recipe != nullptr);
    return WavetableBuilder::build(rate_, *recipe);
  }

  void WavetableBuilder::update(Wavetable& tbl, std::shared_ptr<WavetableRecipe> recipe)
  {
    assert(recipe != nullptr);

    if (needResize(tbl, rate_, *recipe))
      tbl.resize( recipe->preferredPages(rate_),
                  recipe->preferredBasePageSize(rate_),
                  recipe->preferredPageResize(rate_),
                  recipe->preferredBase(rate_),
                  recipe->preferredRange(rate_) );

    fillTable(tbl, rate_, *recipe);
  }

  //static
  Wavetable WavetableBuilder::build(SampleRate rate, const WavetableRecipe& recipe)
  {
    Wavetable tbl {
      recipe.preferredPages(rate),
      recipe.preferredBasePageSize(rate),
      recipe.preferredPageResize(rate),
      recipe.preferredBase(rate),
      recipe.preferredRange(rate)
    };

    WavetableBuilder::fillTable(tbl, rate, recipe);

    return tbl;
  }

  //static
  bool WavetableBuilder::needResize(const Wavetable& tbl, SampleRate rate,
                                    const WavetableRecipe& recipe)
  {
    return tbl.size() != recipe.preferredPages(rate)
      || tbl.pageSize(0) != recipe.preferredBasePageSize(rate)
      || tbl.pageResize() != recipe.preferredPageResize(rate)
      || tbl.base() != recipe.preferredBase(rate)
      || tbl.range() != recipe.preferredRange(rate);
  }

  //static
  void WavetableBuilder::fillTable(Wavetable& tbl, SampleRate rate, const WavetableRecipe& recipe)
  {
    for (size_t page_index = 0; page_index < tbl.size(); ++page_index)
    {
      recipe.fillPage(
        rate,
        page_index,
        tbl.base(page_index),
        tbl[page_index].begin(),
        tbl[page_index].end());
    }
  }


}//namespace audio
