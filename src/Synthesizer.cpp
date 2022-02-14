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

#include "Synthesizer.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>
#include <cassert>

namespace audio {

namespace synth {

  constexpr SampleFormat defaultSampleFormat()
  {
    if (hostEndian() == Endian::kLittle)
      return SampleFormat::kFloat32LE;
    else
      return SampleFormat::kFloat32BE;
  }

  auto viewSamples(ByteBuffer& buffer)
  { return audio::viewSamples<defaultSampleFormat()>(buffer); }

  auto viewSamples(const ByteBuffer& buffer)
  { return audio::viewSamples<defaultSampleFormat()>(buffer); }

  auto viewFrames(ByteBuffer& buffer)
  { return audio::viewFrames<defaultSampleFormat()>(buffer); }

  auto viewFrames(const ByteBuffer& buffer)
  { return audio::viewFrames<defaultSampleFormat()>(buffer); }

  void addNoise(ByteBuffer& buffer, NoiseType type)
  {
    assert(isFloatingPoint(buffer.spec().format));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0, 1.0);

    auto frames = viewFrames(buffer);
    for (auto& frame : frames)
    {
      frame += {dis(gen), dis(gen)};
      frame /= 2.0;
    }
  }

  void addSine(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float sine = std::sin(omega * frame_index);
      frames[frame_index] += amplitude * sine;
      frames[frame_index] /= 2.0;
    }
  }

  void addTriangle(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float triangle = 2.0 * std::asin(std::sin(omega * frame_index)) / M_PI;
      frames[frame_index] += amplitude * triangle;
      frames[frame_index] /= 2.0;
    }
  }

  void addSawtooth(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float time = (omega * frame_index) / (2.0 * M_PI);
      float sawtooth = 2.0 * (time - std::floor(time) - 0.5);
      frames[frame_index] += amplitude * sawtooth;
      frames[frame_index] /= 2.0;
    }
  }

  void addSquare(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float sine = std::sin(omega * frame_index);
      if (sine >= 0)
        frames[frame_index] += amplitude;
      else
        frames[frame_index] -= amplitude;
      frames[frame_index] /= 2.0;
    }
  }

  void addOscillator(ByteBuffer& buffer, const std::vector<Oscillator>& oscillators)
  {
    for (const auto& osc : oscillators)
    {
      switch (osc.shape) {
      case Waveform::kSine:
        addSine(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kTriangle:
        addTriangle(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kSawtooth:
        addSawtooth(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kSquare:
        addSquare(buffer, osc.frequency, osc.amplitude);
        break;
      }
    }
  }

  void applyEnvelope(ByteBuffer& buffer, const Envelope& envelope)
  {
    // not implemented yet
  }

  void applyGain(ByteBuffer& buffer, float gain)
  {
    gain = std::clamp<float>(gain, 0.0, 1.0);
    auto frames = viewFrames(buffer);
    for (auto& frame : frames)
      frame *= gain;
  }

  ByteBuffer mixBuffers(ByteBuffer buffer1, const ByteBuffer& buffer2)
  {
    auto frames1 = viewFrames(buffer1);
    auto frames2 = viewFrames(buffer2);

    auto it2 = frames2.begin();
    for (auto it1 = frames1.begin();
         it1 != frames1.end() && it2 != frames2.end();
         ++it1, ++it2)
    {
      *it1 += *it2;
      *it1 /= 2.0;
    }
    return buffer1;
  }

  ByteBuffer generateSound(double frequency,
                           double volume,
                           double balance,
                           StreamSpec spec,
                           microseconds duration)
  {
    assert(spec.channels == 2);
    assert(!std::isnan(frequency) && frequency>0);
    assert(!std::isnan(volume));
    assert(!std::isnan(balance));

    volume = std::clamp(volume, 0.0, 1.0);
    balance = std::clamp(balance, -1.0, 1.0);

    float balance_l = (balance > 0) ? -1 * balance + 1 : 1;
    float balance_r = (balance < 0) ?  1 * balance + 1 : 1;

    const StreamSpec buffer_spec =
    {
      defaultSampleFormat(),
      spec.rate,
      spec.channels
    };

    ByteBuffer buffer(buffer_spec, duration);

    if (volume > 0)
    {
      float sine_fac = 2 * M_PI * frequency / spec.rate;
      auto n_frames = buffer.frames();
      float one_over_n_frames = 1. / n_frames;
      float volume_drop_exp = 2. / volume;

      auto frames = viewFrames(buffer);
      for(size_t frame = 0; frame < n_frames; ++frame)
      {
        float v = volume * std::pow( 1.0 - one_over_n_frames * frame, volume_drop_exp );
        float amplitude = v * std::sin(sine_fac * frame);

        frames[frame] = { balance_l * amplitude, balance_r * amplitude };
      }
    }
    return resample(buffer, spec);
  }

}//namespace synth
}//namespace audio
