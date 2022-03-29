/*
 * Copyright (C) 2021, 2022 The GMetronome Team
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

#ifndef GMetronome_Synthesizer_h
#define GMetronome_Synthesizer_h

#include "Audio.h"
#include "AudioBuffer.h"

namespace audio {

  struct SoundParameters
  {
    float timbre  {-1.0}; // [-1.0, 1.0]
    float pitch   {800};  // [20.0, 20000.0] (hertz)
    float volume  {1.0};  // [0.0, 100.0] (percent)
    float balance {0.0};  // [-1.0f, 1.0f]
  };

  inline bool operator==(const SoundParameters& lhs, const SoundParameters& rhs)
  {
    return lhs.timbre == rhs.timbre
      && lhs.pitch == rhs.pitch
      && lhs.volume == rhs.volume
      && lhs.balance == rhs.balance;
  }

  inline bool operator!=(const SoundParameters& lhs, const SoundParameters& rhs)
  { return !(lhs==rhs); }

  /**
   * Without mixing capabilities the time gap between two consecutive clicks
   * at maximum tempo (250 bpm) with the maximum number of beat division (4)
   * limits the click sound duration to 60 ms.
   */
  constexpr milliseconds kSoundDuration = 60ms;

  class SoundGenerator {
  public:
    SoundGenerator(const StreamSpec& spec = kDefaultSpec);

    /** Pre-allocate resources */
    void prepare(const StreamSpec& spec);

    /**
     * Allocates a new sound buffer and generates a sound with the given
     * sound parameters.
     */
    ByteBuffer generate(const SoundParameters& params);

    /**
     * Generates a sound with the given sound parameters. The buffer should be
     * able to hold 60ms (kSoundDuration) of audio data.
     * If the stream specification of the buffer does not fit the specification
     * of the SoundGenerator, the buffer will be reinterpreted but no further
     * heap allocations will take place to comply real-time demands.
     */
    void generate(ByteBuffer& buffer, const SoundParameters& params);

    /** See generate(const SoundParameter&) */
    ByteBuffer operator()(const SoundParameters& params)
      { return generate(params); }

    /** See generate(ByteBuffer&, const SoundParameter&) */
    void operator()(ByteBuffer& buffer, const SoundParameters& params)
      { generate(buffer, params); }

  private:
    StreamSpec spec_;
    ByteBuffer osc_buffer_;
    ByteBuffer noise_buffer_;
  };

}//namespace audio
#endif//GMetronome_Synthesizer_h
