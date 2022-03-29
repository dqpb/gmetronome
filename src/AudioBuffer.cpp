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
# include <iostream>
#endif

namespace audio {

  void ByteBuffer::resample(const StreamSpec& spec)
  {
    ByteBuffer resampled = ::audio::resample(*this, spec);
    (*this) = std::move(resampled);
  }

  template<typename Frames>
  void resample(const ByteBuffer& src_buffer, Frames tgt_frames)
  {
    using Fmt = SampleFormat;

    switch(src_buffer.spec().format)
    {
    case Fmt::kU8: tgt_frames = viewFrames<Fmt::kU8>(src_buffer); break;
    case Fmt::kS8: tgt_frames = viewFrames<Fmt::kS8>(src_buffer); break;
    case Fmt::kS16LE: tgt_frames = viewFrames<Fmt::kS16LE>(src_buffer); break;
    case Fmt::kS16BE: tgt_frames = viewFrames<Fmt::kS16BE>(src_buffer); break;
    case Fmt::kU16LE: tgt_frames = viewFrames<Fmt::kU16LE>(src_buffer); break;
    case Fmt::kU16BE: tgt_frames = viewFrames<Fmt::kU16BE>(src_buffer); break;
    case Fmt::kS32LE: tgt_frames = viewFrames<Fmt::kS32LE>(src_buffer); break;
    case Fmt::kS32BE: tgt_frames = viewFrames<Fmt::kS32BE>(src_buffer); break;
    case Fmt::kFloat32LE: tgt_frames = viewFrames<Fmt::kFloat32LE>(src_buffer); break;
    case Fmt::kFloat32BE: tgt_frames = viewFrames<Fmt::kFloat32BE>(src_buffer); break;
    case Fmt::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "AudioBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };
  }

  void resample(const ByteBuffer& src_buffer, ByteBuffer& tgt_buffer)
  {
    const auto& src_spec = src_buffer.spec();
    const auto& tgt_spec = tgt_buffer.spec();

    // TODO: this should support channel and rate resampling too
    assert(src_spec.channels == tgt_spec.channels);
    assert(src_spec.rate == tgt_spec.rate);

    if (&src_buffer == &tgt_buffer)
      return;

#ifndef NDEBUG
    if (src_buffer.frames() > tgt_buffer.frames())
      std::cerr << "AudioBuffer: target buffer too small for resampling" << std::endl;
#endif

    using Fmt = SampleFormat;

    switch(tgt_spec.format)
    {
    case Fmt::kS8:  resample( src_buffer, viewFrames<Fmt::kS8>(tgt_buffer) ); break;
    case Fmt::kU8:  resample( src_buffer, viewFrames<Fmt::kU8>(tgt_buffer) ); break;
    case Fmt::kS16LE:  resample( src_buffer, viewFrames<Fmt::kS16LE>(tgt_buffer) ); break;
    case Fmt::kS16BE:  resample( src_buffer, viewFrames<Fmt::kS16BE>(tgt_buffer) ); break;
    case Fmt::kU16LE:  resample( src_buffer, viewFrames<Fmt::kU16LE>(tgt_buffer) ); break;
    case Fmt::kU16BE:  resample( src_buffer, viewFrames<Fmt::kU16BE>(tgt_buffer) ); break;
    case Fmt::kS32LE:  resample( src_buffer, viewFrames<Fmt::kS32LE>(tgt_buffer) ); break;
    case Fmt::kS32BE:  resample( src_buffer, viewFrames<Fmt::kS32BE>(tgt_buffer) ); break;
    case Fmt::kFloat32LE: resample( src_buffer, viewFrames<Fmt::kFloat32LE>(tgt_buffer) ); break;
    case Fmt::kFloat32BE: resample( src_buffer, viewFrames<Fmt::kFloat32BE>(tgt_buffer) ); break;
    case Fmt::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "AudioBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };
  }

  ByteBuffer resample(const ByteBuffer& src_buffer, const StreamSpec& tgt_spec)
  {
    ByteBuffer tgt_buffer(tgt_spec, src_buffer.frames() * frameSize(tgt_spec));
    resample(src_buffer, tgt_buffer);
    return tgt_buffer;
  }

}//namespace audio
