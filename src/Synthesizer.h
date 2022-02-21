/*
 * Copyright (C) 2021 The GMetronome Team
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
#include <vector>

namespace audio {
namespace synth {

  struct AutomationPoint
  {
    microseconds time;
    float value;
  };

  using Automation = std::vector<AutomationPoint>;

  enum class Waveform
  {
    kSine,
    kTriangle,
    kSawtooth,
    kSquare
  };

  struct Oscillator
  {
    float frequency;
    float amplitude;
    Waveform shape;
  };

  void addNoise (ByteBuffer& buffer, float amplitude);
  void addSine (ByteBuffer& buffer, float frequency, float amplitude);
  void addTriangle (ByteBuffer& buffer, float frequency, float amplitude);
  void addSawtooth (ByteBuffer& buffer, float frequency, float amplitude);
  void addSquare (ByteBuffer& buffer, float frequency, float amplitude);
  void addOscillator (ByteBuffer& buffer, const std::vector<Oscillator>& oscillators);
  void applyGain (ByteBuffer& buffer, float gain_l, float gain_r);
  void applyGain (ByteBuffer& buffer, float gain);
  void applyGain (ByteBuffer& buffer, const Automation& gain);
  void normalize (ByteBuffer& buffer, float gain_l, float gain_r);
  void normalize (ByteBuffer& buffer, float gain);
  void applySmoothing (ByteBuffer& buffer, const Automation& kernel_width);
  void applySmoothing (ByteBuffer& buffer, microseconds kernel_width);
  ByteBuffer mixBuffers (ByteBuffer buffer1, const ByteBuffer& buffer2);

  ByteBuffer generateClick(const StreamSpec& spec,
                           float timbre = -1.0,
                           float pitch = 800,
                           float volume = 1.0,
                           float balance = 0.0);

  using SoundPackage = std::vector<ByteBuffer>;

  /**
   * @brief Generates a collection of slightly different sounds.
   *
   * This function calls generateClick() to create the sounds, but
   * tries to parallelize the sound generation in accordance with
   * the system capabilities.
   */
  SoundPackage generateClickPackage(const StreamSpec& spec,
                                    float timbre = -1.0,
                                    float pitch = 800,
                                    float volume = 1.0,
                                    float balance = 0.0,
                                    std::size_t package_size = 3);
}//namespace synth
}//namespace audio
#endif//GMetronome_Synthesizer_h
