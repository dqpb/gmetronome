/*
 * Copyright (C) 2020-2022 The GMetronome Team
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

#include "AudioBuffer.h"
#include <cassert>

#ifndef NDEBUG
#  include <iostream>
#endif

namespace audio {

  template<typename SrcChannels, typename TgtChannels>
  void resample(SrcChannels src_chs, TgtChannels tgt_chs, const ChannelMap& map)
  {
    int map_size = static_cast<int>(map.size());
    int src_size = static_cast<int>(src_chs.size());
    int tgt_size = static_cast<int>(tgt_chs.size());

    for (int src_idx = 0; src_idx < src_size; ++src_idx)
    {
      int tgt_idx = (map_size > src_idx) ? map[src_idx] : src_idx;

      if (tgt_idx >= 0 && tgt_idx < tgt_size)
        tgt_chs[tgt_idx] = src_chs[src_idx];
    }
  }

  template<typename TgtChannels>
  void resample(const ByteBuffer& src_buf, TgtChannels tgt_chs, const ChannelMap& map)
  {
    using Fmt = SampleFormat;

    switch(src_buf.spec().format)
    {
    case Fmt::kU8: resample(viewChannels<Fmt::kU8>(src_buf), tgt_chs, map); break;
    case Fmt::kS8: resample(viewChannels<Fmt::kS8>(src_buf), tgt_chs, map); break;
    case Fmt::kS16LE: resample(viewChannels<Fmt::kS16LE>(src_buf), tgt_chs, map); break;
    case Fmt::kS16BE: resample(viewChannels<Fmt::kS16BE>(src_buf), tgt_chs, map); break;
    case Fmt::kU16LE: resample(viewChannels<Fmt::kU16LE>(src_buf), tgt_chs, map); break;
    case Fmt::kU16BE: resample(viewChannels<Fmt::kU16BE>(src_buf), tgt_chs, map); break;
    case Fmt::kS32LE: resample(viewChannels<Fmt::kS32LE>(src_buf), tgt_chs, map); break;
    case Fmt::kS32BE: resample(viewChannels<Fmt::kS32BE>(src_buf), tgt_chs, map); break;
    case Fmt::kFloat32LE: resample(viewChannels<Fmt::kFloat32LE>(src_buf), tgt_chs, map); break;
    case Fmt::kFloat32BE: resample(viewChannels<Fmt::kFloat32BE>(src_buf), tgt_chs, map); break;
    case Fmt::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "AudioBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };
  }

  void resample(const ByteBuffer& src_buf, ByteBuffer& tgt_buf, const ChannelMap& map)
  {
#ifndef NDEBUG
    const auto& src_spec = src_buf.spec();
#endif
    const auto& tgt_spec = tgt_buf.spec();

    // TODO: support rate resampling
    assert(src_spec.rate == tgt_spec.rate);

    if (&src_buf == &tgt_buf)
      return;

#ifndef NDEBUG
    if (src_buf.frames() > tgt_buf.frames())
      std::cerr << "AudioBuffer: target buffer too small for resampling" << std::endl;
#endif

    using Fmt = SampleFormat;

    switch(tgt_spec.format)
    {
    case Fmt::kS8: resample(src_buf, viewChannels<Fmt::kS8>(tgt_buf), map); break;
    case Fmt::kU8: resample(src_buf, viewChannels<Fmt::kU8>(tgt_buf), map); break;
    case Fmt::kS16LE: resample(src_buf, viewChannels<Fmt::kS16LE>(tgt_buf), map); break;
    case Fmt::kS16BE: resample(src_buf, viewChannels<Fmt::kS16BE>(tgt_buf), map); break;
    case Fmt::kU16LE: resample(src_buf, viewChannels<Fmt::kU16LE>(tgt_buf), map); break;
    case Fmt::kU16BE: resample(src_buf, viewChannels<Fmt::kU16BE>(tgt_buf), map); break;
    case Fmt::kS32LE: resample(src_buf, viewChannels<Fmt::kS32LE>(tgt_buf), map); break;
    case Fmt::kS32BE: resample(src_buf, viewChannels<Fmt::kS32BE>(tgt_buf), map); break;
    case Fmt::kFloat32LE: resample(src_buf, viewChannels<Fmt::kFloat32LE>(tgt_buf), map); break;
    case Fmt::kFloat32BE: resample(src_buf, viewChannels<Fmt::kFloat32BE>(tgt_buf), map); break;
    case Fmt::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "AudioBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };
  }

}//namespace audio
