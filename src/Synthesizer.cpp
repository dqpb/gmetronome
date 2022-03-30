/*
 * Copyright (C) 2021,2022 The GMetronome Team
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Filter.h"
#include "Synthesizer.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cassert>

#ifndef NDEBUG
#  include <iostream>
#endif

namespace audio {

  SoundGenerator::SoundGenerator(const StreamSpec& spec)
  { prepare(spec); }

  void SoundGenerator::prepare(const StreamSpec& spec)
  {
    assert(spec.rate > 0);
    assert(spec.channels == 2);

    const StreamSpec filter_buffer_spec =
      { filter::kDefaultSampleFormat, spec.rate, 2 };

    osc_buffer_.resize(filter_buffer_spec, kSoundDuration);
    noise_buffer_.resize(filter_buffer_spec, kSoundDuration);

    spec_ = spec;
  }

  ByteBuffer SoundGenerator::generate(const SoundParameters& params)
  {
    ByteBuffer buffer(spec_, kSoundDuration);
    generate(buffer, params);
    return buffer;
  }

  void SoundGenerator::generate(ByteBuffer& buffer, const SoundParameters& params)
  {
    assert(!std::isnan(params.timbre));
    assert(!std::isnan(params.pitch));
    assert(!std::isnan(params.volume));
    assert(!std::isnan(params.balance));

    if (buffer.spec() != spec_)
    {
#ifndef NDEBUG
      std::cerr << "SoundGenerator: incompatible buffer spec (reinterpreting buffer)"
                << std::endl;
#endif
      buffer.reinterpret(spec_);
    }

#ifndef NDEBUG
    if (buffer.frames() < usecsToFrames(kSoundDuration, buffer.spec()))
      std::cerr << "SoundGenerator: target buffer too small (sound will be truncated)"
                << std::endl;
#endif

    float timbre = std::clamp(params.timbre, -1.0f, 1.0f);
    float pitch = std::clamp(params.pitch, 20.0f, 20000.0f);
    float volume = std::clamp(params.volume, 0.0f, 1.0f);
    float balance = std::clamp(params.balance, -1.0f, 1.0f);

    if (volume > 0)
    {
      const std::vector<filter::Oscillator> oscillators = {
        {pitch,         1.0f, filter::Waveform::kSine},
        // {pitch * 0.67f, 0.6f, filter::Waveform::kSine},
        // {pitch * 1.32f, 0.6f, filter::Waveform::kSine},
        // {pitch * 0.43f, 0.3f, filter::Waveform::kSine},

        // {pitch * 1.68f, 0.3f, filter::Waveform::kSine},
        // {pitch * 2.68f, 0.1f, filter::Waveform::kSine}

        {pitch *  3.0f / 2.0f, 0.2f, filter::Waveform::kSine},
        {pitch *  5.0f / 4.0f, 0.1f, filter::Waveform::kSine},
        //{pitch * 15.0f / 8.0f, 0.05f, filter::Waveform::kSquare}
      };

      static const filter::Automation osc_envelope = {
        {0ms,  0.0},
        {3ms,  0.7},
        {7ms,  0.3},
        {8ms,  0.7},
        {12ms, 0.15},
        {30ms, 0.05},
        {60ms, 0.0}
      };

      float osc_gain = (timbre + 1.0) / 2.0;

      auto osc_filter =
          filter::std::Osc {oscillators}
        | filter::std::Gain {osc_gain}
        | filter::std::Gain {osc_envelope};

      // apply filter
      osc_filter(osc_buffer_);

      float noise_gain = std::max(-timbre, 0.0f);

      // smoothing kernel width
      const filter::Automation noise_smooth_kw = {
        {0ms,  static_cast<float>(usecsToFrames(0us, noise_buffer_.spec()))},
        {1ms,  static_cast<float>(usecsToFrames(20us, noise_buffer_.spec()))},
        {5ms,  static_cast<float>(usecsToFrames(70us, noise_buffer_.spec()))},
        {6ms,  static_cast<float>(usecsToFrames(20us, noise_buffer_.spec()))},
        {11ms, static_cast<float>(usecsToFrames(70us, noise_buffer_.spec()))},
        //{30ms, static_cast<float>(usecsToFrames(30us, noise_buffer_.spec()))},
        //{60ms, static_cast<float>(usecsToFrames(70us, noise_buffer_.spec()))},
      };

      static const filter::Automation noise_envelope = {
        {0ms,  0.0},
        {1ms,  1.0},
        {5ms,  0.1},
        {6ms,  1.0},
        {11ms, 0.1},
        {60ms, 0.0}
      };

      float balance_l = (balance > 0) ? volume * (-1.0 * balance + 1.0) : volume;
      float balance_r = (balance < 0) ? volume * ( 1.0 * balance + 1.0) : volume;

      // static const filter::Automation final_smooth_kw = {
      //   {0ms,  static_cast<float>(usecsToFrames(0us, noise_buffer_.spec()))},
      //   {30ms, static_cast<float>(usecsToFrames(0us, noise_buffer_.spec()))},
      //   {60ms, static_cast<float>(usecsToFrames(200us, noise_buffer_.spec()))},
      // };

      auto noise_filter =
          filter::std::Noise {noise_gain}
        | filter::std::Smooth {noise_smooth_kw}
        | filter::std::Gain {noise_envelope}
        | filter::std::Mix {osc_buffer_}
        | filter::std::Normalize {balance_l, balance_r};

      // apply filter
      noise_filter(noise_buffer_);
    }
    return resample(noise_buffer_, buffer);
  }

}//namespace audio
