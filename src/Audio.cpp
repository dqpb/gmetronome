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
#include <cmath>

namespace audio {

  bool operator==(const StreamSpec& lhs, const StreamSpec& rhs)
  {
    return lhs.format == rhs.format
      && lhs.rate == rhs.rate
      && lhs.channels == rhs.channels;
  }

  bool operator!=(const StreamSpec& lhs, const StreamSpec& rhs)
  { return !(lhs==rhs); }

  size_t frameSize(const StreamSpec& spec) {
    return sampleSize(spec.format) * spec.channels;
  }

  size_t usecsToFrames(microseconds usecs, const StreamSpec& spec) {
    return (size_t) ((std::chrono::abs(usecs).count() * spec.rate) / std::micro::den);
  }

  microseconds framesToUsecs(size_t frames, const StreamSpec& spec) {
    return microseconds((frames * std::micro::den) / spec.rate);
  }

  size_t usecsToBytes(microseconds usecs, const StreamSpec& spec) {
    return usecsToFrames(usecs, spec) * frameSize(spec);
  }

  microseconds bytesToUsecs(size_t bytes, const StreamSpec& spec) {
    size_t nframes = bytes / frameSize(spec);
    return framesToUsecs(nframes, spec);
  }

}//namespace audio
