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

    float timbre      = std::clamp(params.timbre, -1.0f, 1.0f);
    float pitch       = std::clamp(params.pitch, 20.0f, 20000.0f);
    bool  bell        = params.bell;
    float bell_volume = std::clamp(params.bell_volume, 0.0f, 1.0f);
    float balance     = std::clamp(params.balance, -1.0f, 1.0f);
    float volume      = std::clamp(params.volume, 0.0f, 1.0f);

    if (volume == 0)
    {
      filter::std::Zero{}(buffer);
      return;
    }

    float osc_gain = (timbre + 1.0f) / 2.0f;
    float osc_attack_gain = std::abs((timbre - 1.0f) / 2.0f);
    float noise_gain = std::max(-timbre, 0.0f);

    static const filter::Automation attack_envelope = {
       {8ms,  0.0},
       {13ms, 1.0},
       {14ms, 0.3},
       {20ms, 0.0}
     };

    const StreamSpec filter_buffer_spec =
      { filter::kDefaultSampleFormat, spec_.rate, 2 };

    ByteBuffer osc_attack_buffer {filter_buffer_spec, kSoundDuration};

    auto osc_attack_filter =
        filter::std::Square {pitch * 2.1f, osc_attack_gain * 0.4f}
      | filter::std::Square {pitch * 7.6f, osc_attack_gain * 0.2f}
      | filter::std::Gain {attack_envelope};

    osc_attack_filter(osc_attack_buffer);

    static const filter::Automation osc_envelope = {
      {8ms,  0.0},
      {13ms, 1.0},
      {25ms, 0.1},
      {60ms, 0.0}
    };


    auto osc_filter =
      filter::std::Zero {}
      | filter::std::Sine {pitch, osc_gain}
      | filter::std::Mix {osc_attack_buffer}
      | filter::std::Gain {osc_envelope};

    // apply filter
    osc_filter(osc_buffer_);


    ByteBuffer bell_buffer {filter_buffer_spec, kSoundDuration};

    if (bell)
    {
      static const filter::Automation bell_envelope = {
        { 2ms, 0.0},
        { 3ms, 1.0},
        {60ms, 0.0}
      };

      auto bell_filter =
          filter::std::Sine {7000, 0.3f}
        | filter::std::Gain {bell_envelope};

      bell_filter(bell_buffer);
    }

    // smoothing kernel width
    const filter::Automation noise_smooth_kw = {
      { 0ms, static_cast<float>(usecsToFrames(200us, noise_buffer_.spec()))},
      { 5ms, static_cast<float>(usecsToFrames(  0us, noise_buffer_.spec()))},
      { 8ms, static_cast<float>(usecsToFrames(100us, noise_buffer_.spec()))},
      {10ms, static_cast<float>(usecsToFrames(  0us, noise_buffer_.spec()))},
      {13ms, static_cast<float>(usecsToFrames(100us, noise_buffer_.spec()))},
      {60ms, static_cast<float>(usecsToFrames(100us, noise_buffer_.spec()))},
    };

    static const filter::Automation noise_envelope = {
      { 0ms, 0.0},
      { 3ms, 0.2},
      { 4ms, 0.5},
      { 5ms, 1.0},
      { 7ms, 0.1},
      {10ms, 1.0},
      {12ms, 0.1},
      {60ms, 0.0}
    };

    float balance_l = (balance > 0) ? volume * (-1.0 * balance + 1.0) : volume;
    float balance_r = (balance < 0) ? volume * ( 1.0 * balance + 1.0) : volume;

    auto noise_filter =
      filter::std::Zero {}
      | filter::std::Noise {noise_gain}
      | filter::std::Smooth {noise_smooth_kw}
      | filter::std::Gain {noise_envelope}
      | filter::std::Mix {osc_buffer_}
      | filter::std::Mix {bell_buffer}
      | filter::std::Normalize {balance_l, balance_r};

    // apply filter
    noise_filter(noise_buffer_);
    resample(noise_buffer_, buffer);
  }

}//namespace audio
