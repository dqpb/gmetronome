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
#include "Filter.h"
#include "AudioBuffer.h"
#include "WavetableLibrary.h"

#include <tuple>

namespace audio {

  enum class EnvelopeRampShape
  {
    kLinear = 1,
    // kQuadratic = 2,
    // kQuadraticFlipped = 3,
    kCubic = 4,
    kCubicFlipped = 5
  };

  enum class EnvelopeHoldShape
  {
    kKeep = 0,
    // kLinear = 1,
    // kQuadratic = 2,
    // kCubic = 3,
    kQuartic = 4
  };

  struct SoundParameters
  {
    float             tone_pitch        {1000.0f};                     // [40.0f, 10000.0f] (hertz)
    float             tone_timbre       {0.0f};                        // [0.0f, 3.0f]
    float             tone_detune       {0.0f};                        // [0.0f, 100.0f] (cents)
    float             tone_attack       {10.0f};                       // [0.0f, 20.0f] (ms)
    EnvelopeRampShape tone_attack_shape {EnvelopeRampShape::kLinear};
    float             tone_hold         {10.0f};                       // [0.0f, 20.0f] (ms)
    EnvelopeHoldShape tone_hold_shape   {EnvelopeHoldShape::kKeep};
    float             tone_decay        {10.0f};                       // [0.0f, 20.0f] (ms)
    EnvelopeRampShape tone_decay_shape  {EnvelopeRampShape::kLinear};

    float             percussion_cutoff       {1000.0f};               // [40.0f, 10000.0f] (hertz)
    float             percussion_attack       {10.0f};                 // [0.0f, 20.0f] (ms)
    EnvelopeRampShape percussion_attack_shape {EnvelopeRampShape::kLinear};
    float             percussion_hold         {10.0};                  // [0.0f, 20.0f] (ms)
    EnvelopeHoldShape percussion_hold_shape   {EnvelopeHoldShape::kKeep};
    float             percussion_decay        {10.0f};                 // [0.0f, 20.0f] (ms)
    EnvelopeRampShape percussion_decay_shape  {EnvelopeRampShape::kLinear};

    float mix     {-100.0f}; // [-100.0f, 100.0f] (percent)
    float pan     {0.0f};    // [-100.0f, 100.0f] (percent)
    float volume  {75.0f};   // [   0.0f, 100.0f] (percent)
  };

  /**
   * Without mixing capabilities the time gap between two consecutive clicks
   * at maximum tempo (250 bpm) with the maximum number of beat division (4)
   * limits the click sound duration to 60 ms.
   */
  constexpr milliseconds kSoundDuration = 60ms;

  /**
   * @class Synthesizer
   * @brief Implements the requirements of a builder for ObjectLibrary.
   */
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
    ByteBuffer noise_buffer_;

    // wavetable library keys
    static constexpr int kSineTable     = 0;
    static constexpr int kTriangleTable = 1;
    static constexpr int kSawtoothTable = 2;
    static constexpr int kSquareTable   = 3;

    WavetableLibrary wavetables_;

    using NoiseFilterPipe = decltype(
        filter::std::Zero()
      | filter::std::Noise()
      | filter::std::Lowpass()
      | filter::std::Gain()
    );

    NoiseFilterPipe noise_pipe_;

    using OscFilterPipe = decltype(
        filter::std::Zero()
      | filter::std::Wave() // Sine
      | filter::std::Wave() // Triangle
      | filter::std::Wave() // Sawtooth
      | filter::std::Wave() // Square
      | filter::std::Gain()
      | filter::std::Mix()
    );

    OscFilterPipe osc_pipe_;

    filter::Automation buildEnvelope(float attack, EnvelopeRampShape attack_shape,
                                     float hold, EnvelopeHoldShape hold_shape,
                                     float decay, EnvelopeRampShape decay_shape) const;
  };

}//namespace audio
#endif//GMetronome_Synthesizer_h
