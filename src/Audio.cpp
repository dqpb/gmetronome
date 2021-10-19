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

#include "Audio.h"
#include <map>

namespace audio {

  size_t sampleSize(SampleFormat format) {
    static const std::map<SampleFormat,size_t> _m = {
      {SampleFormat::U8, 1},
      {SampleFormat::ALAW, 1},
      {SampleFormat::ULAW, 1},
      {SampleFormat::S16LE, 2},
      {SampleFormat::S16BE, 2},
      {SampleFormat::Float32LE, 4},
      {SampleFormat::Float32BE, 4},
      {SampleFormat::S32LE, 4},
      {SampleFormat::S32BE, 4},
      {SampleFormat::S24LE, 3},
      {SampleFormat::S24BE, 3},
      {SampleFormat::S24_32LE, 4},
      {SampleFormat::S24_32BE, 4}
    };
    return _m.at(format);
  }

  Endianness endianness(SampleFormat format) {
    static const std::map<SampleFormat,Endianness> _m = {
      {SampleFormat::U8, Endianness::Invalid},
      {SampleFormat::ALAW, Endianness::Invalid},
      {SampleFormat::ULAW, Endianness::Invalid},
      {SampleFormat::S16LE, Endianness::Little},
      {SampleFormat::S16BE, Endianness::Big},
      {SampleFormat::Float32LE, Endianness::Little},
      {SampleFormat::Float32BE, Endianness::Big},
      {SampleFormat::S32LE, Endianness::Little},
      {SampleFormat::S32BE, Endianness::Big},
      {SampleFormat::S24LE, Endianness::Little},
      {SampleFormat::S24BE, Endianness::Big},
      {SampleFormat::S24_32LE, Endianness::Little},
      {SampleFormat::S24_32BE, Endianness::Big}
    };
    return _m.at(format);
  }

  size_t frameSize(SampleSpec spec) {
    return sampleSize(spec.format) * spec.channels;
  }

  size_t usecsToFrames(std::chrono::microseconds usecs, const SampleSpec& spec) {
    return (size_t) ((usecs.count() * spec.rate) / std::micro::den);
  }
  
  std::chrono::microseconds framesToUsecs(size_t frames, const SampleSpec& spec) {
    return std::chrono::microseconds((frames * std::micro::den) / spec.rate);
  }

  size_t usecsToBytes(std::chrono::microseconds usecs, const SampleSpec& spec) {
    return usecsToFrames(usecs, spec) * frameSize(spec);
  }
  
  std::chrono::microseconds bytesToUsecs(size_t bytes, const SampleSpec& spec) {
    size_t nframes = bytes / frameSize(spec);
    return framesToUsecs(nframes, spec);
  }

}//namespace audio
