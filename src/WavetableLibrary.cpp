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
#include <cassert>

namespace audio {

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
