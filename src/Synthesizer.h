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

  enum EnvelopePointShape
  {
    kLinear = 1,
    kPow8   = 8,
    kPow10  = 10
  };

  struct EnvelopePoint
  {
    milliseconds time;
    float amplitude;
    EnvelopePointShape shape;
  };

  using Envelope = std::vector<EnvelopePoint>;

  enum class NoiseType
  {
    kWhite = 0,
    kPink  = 1,
    kBrown = 2
  };

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

  void addNoise(ByteBuffer& buffer, NoiseType type = NoiseType::kWhite);

  void addSine(ByteBuffer& buffer, float frequency, float amplitude);

  void addTriangle(ByteBuffer& buffer, float frequency, float amplitude);

  void addSawtooth(ByteBuffer& buffer, float frequency, float amplitude);

  void addSquare(ByteBuffer& buffer, float frequency, float amplitude);

  void addOscillator(ByteBuffer& buffer, const std::vector<Oscillator>& osc);

  void applyEnvelope(ByteBuffer& buffer, const Envelope& envelope);

  void applyGain(ByteBuffer& buffer, float gain);

  ByteBuffer mixBuffers(ByteBuffer buffer1, const ByteBuffer& buffer2);

  ByteBuffer generateSound(double frequency,
                           double volume,
                           double balance,
                           StreamSpec spec,
                           microseconds duration);

}//namespace synth
}//namespace audio
#endif//GMetronome_Synthesizer_h
