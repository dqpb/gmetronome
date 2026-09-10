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
#include <cassert>

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

  //static
  Decibel Decibel::fromVolume(double volume, const Decibel& min, const Decibel& max) noexcept
  {
    assert(min <= max);

    if (volume <= 0.0)
      return mute();

    if (volume >= 100.0)
      return max;

    if (min.isMute())
      return fromLinear( (volume / 100.0) * max.linear() );

    return Decibel( min.value_ + (volume / 100.0) *  (max.value_ - min.value_) );
  }

  double Decibel::volume(const Decibel& min, const Decibel& max) noexcept
  {
    assert(min <= max);

    if (value_ <= min.value_)
      return 0.0;

    if (value_ >= max.value_)
      return 100.0;

    if (min.isMute())
    {
      if (double max_gain = max.linear(); max_gain <= 0.0)
        return 0.0;
      else
        return std::clamp((linear() / max_gain) * 100.0, 0.0, 100.0);
    }

    if (double range = max.value_ - min.value_; range == 0.0)
      return linear() <= min.linear() ? 0.0 : 100.0;
    else
      return std::clamp(((value_ - min.value_) / range) * 100.0, 0.0, 100.0);
  }
}//namespace audio
