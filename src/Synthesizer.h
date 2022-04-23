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

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  struct SoundParameters
  {
    float pitch        {900};    // [20.0f, 20000.0f] (hertz)
    float timbre       {1.0};    // [-1.0f, 1.0f]
    float detune       {0.5};    // [0.0f, 1.0f]
    float punch        {0.5};    // [0.0f, 1.0f]
    float decay        {0.5};    // [0.0f, 1.0f]
    bool  bell         {false};
    float bell_volume  {1.0};    // [ 0.0f, 1.0f]
    float balance      {0.0};    // [-1.0f, 1.0f]
    float volume       {1.0};    // [ 0.0f, 1.0f]

#ifndef NDEBUG
    friend std::ostream& operator<<(std::ostream& o, const SoundParameters& params)
      {
        o << "["
          << params.pitch << ", "
          << params.timbre << ", "
          << params.detune << ", "
          << params.punch << ", "
          << params.decay << ", "
          << params.bell << ", "
          << params.bell_volume << ", "
          << params.balance << ", "
          << params.volume
          << "]";
        return o;
      }
#endif
  };

  inline bool operator==(const SoundParameters& lhs, const SoundParameters& rhs)
  {
    return lhs.pitch == rhs.pitch
      && lhs.timbre == rhs.timbre
      && lhs.detune == rhs.detune
      && lhs.punch == rhs.punch
      && lhs.decay == rhs.decay
      && lhs.bell == rhs.bell
      && lhs.bell_volume == rhs.bell_volume
      && lhs.balance == rhs.balance
      && lhs.volume == rhs.volume;
  }

  inline bool operator!=(const SoundParameters& lhs, const SoundParameters& rhs)
  { return !(lhs==rhs); }

  /**
   * Without mixing capabilities the time gap between two consecutive clicks
   * at maximum tempo (250 bpm) with the maximum number of beat division (4)
   * limits the click sound duration to 60 ms.
   */
  constexpr milliseconds kSoundDuration = 60ms;

  /** Implements the requirements of a builder for ObjectLibrary. */
  class Synthesizer {
  public:
    Synthesizer(const StreamSpec& spec = kDefaultSpec);

    /** Pre-allocate resources */
    void prepare(const StreamSpec& spec);

    /**
     * Allocates a new sound buffer and generates a sound with the given
     * sound parameters.
     */
    ByteBuffer create(const SoundParameters& params);

    /**
     * Generates a sound with the given sound parameters. The buffer should be
     * able to hold 60ms (kSoundDuration) of audio data.
     * If the stream specification of the buffer does not fit the specification
     * of the Synthesizer, the buffer will be resized.
     */
    void update(ByteBuffer& buffer, const SoundParameters& params);

  private:
    StreamSpec spec_;
    ByteBuffer osc_buffer_;
    ByteBuffer att_buffer_;
    ByteBuffer noise_buffer_;
  };

}//namespace audio
#endif//GMetronome_Synthesizer_h
