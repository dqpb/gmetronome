/*
 * Copyright (C) 2020 The GMetronome Team
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

#include "AudioBuffer.h"
#include <cassert>
#include <iostream>

namespace audio {

  void ByteBuffer::resample(const StreamSpec& spec)
  {
    ByteBuffer resampled = ::audio::resample(*this, spec);
    (*this) = std::move(resampled);
  }

  template<typename Frames>
  void resample(Frames to_frames, const ByteBuffer& from_buf)
  {
    using SF = SampleFormat;

    switch(from_buf.spec().format)
    {
    case SF::kU8: to_frames = viewFrames<SF::kU8>(from_buf); break;
    case SF::kS8: to_frames = viewFrames<SF::kS8>(from_buf); break;
    case SF::kS16LE: to_frames = viewFrames<SF::kS16LE>(from_buf); break;
    case SF::kS16BE: to_frames = viewFrames<SF::kS16BE>(from_buf); break;
    case SF::kU16LE: to_frames = viewFrames<SF::kU16LE>(from_buf); break;
    case SF::kU16BE: to_frames = viewFrames<SF::kU16BE>(from_buf); break;
    case SF::kS32LE: to_frames = viewFrames<SF::kS32LE>(from_buf); break;
    case SF::kS32BE: to_frames = viewFrames<SF::kS32BE>(from_buf); break;
    case SF::kFloat32LE: to_frames = viewFrames<SF::kFloat32LE>(from_buf); break;
    case SF::kFloat32BE: to_frames = viewFrames<SF::kFloat32BE>(from_buf); break;
    case SF::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "ByteBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };
  }

  ByteBuffer resample(const ByteBuffer& buffer, const StreamSpec& to_spec)
  {
    // TODO: this should support channel and rate resampling too
    assert(buffer.spec().channels == to_spec.channels && "not implemented yet");
    assert(buffer.spec().rate == to_spec.rate && "not implemented yet");

    const ByteBuffer& from_buf = buffer;
    ByteBuffer to_buf(to_spec, from_buf.frames() * frameSize(to_spec));

    using SF = SampleFormat;

    switch(to_spec.format)
    {
    case SF::kS8:  resample( viewFrames<SF::kS8>(to_buf), from_buf ); break;
    case SF::kU8:  resample( viewFrames<SF::kU8>(to_buf), from_buf ); break;
    case SF::kS16LE:  resample( viewFrames<SF::kS16LE>(to_buf), from_buf ); break;
    case SF::kS16BE:  resample( viewFrames<SF::kS16BE>(to_buf), from_buf ); break;
    case SF::kU16LE:  resample( viewFrames<SF::kU16LE>(to_buf), from_buf ); break;
    case SF::kU16BE:  resample( viewFrames<SF::kU16BE>(to_buf), from_buf ); break;
    case SF::kS32LE:  resample( viewFrames<SF::kS32LE>(to_buf), from_buf ); break;
    case SF::kS32BE:  resample( viewFrames<SF::kS32BE>(to_buf), from_buf ); break;
    case SF::kFloat32LE: resample( viewFrames<SF::kFloat32LE>(to_buf), from_buf ); break;
    case SF::kFloat32BE: resample( viewFrames<SF::kFloat32BE>(to_buf), from_buf ); break;
    case SF::kUnknown:
      [[fallthrough]];
    default:
#ifndef NDEBUG
      std::cerr << "ByteBuffer: unable to resample (unknown sample format)" << std::endl;
#endif
      break;
    };

    return to_buf;
  }

}//namespace audio
