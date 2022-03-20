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

#include "Filter.h"

#include "Synthesizer.h"
#include <vector>
#include <algorithm>
#include <future>
#include <cmath>
#include <cassert>

namespace audio {
namespace synth {

  ByteBuffer generateClick(const StreamSpec& spec,
                           float timbre,
                           float pitch,
                           float volume,
                           float balance)
  {
    assert(spec.channels == 2);
    assert(!std::isnan(volume));
    assert(!std::isnan(balance));

    static const milliseconds kBufferDuration = 60ms;

    const StreamSpec buffer_spec =
      {
        filter::defaultSampleFormat(),
        spec.rate,
        spec.channels
      };

    ByteBuffer buffer(buffer_spec, kBufferDuration);

    if (volume > 0)
    {
      ByteBuffer osc_buffer(buffer_spec, kBufferDuration);

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
      osc_filter(osc_buffer);

      float noise_gain = std::max(-timbre, 0.0f);

      // smoothing kernel width
      static const filter::Automation noise_smooth_kw = {
        {0ms,  usecsToFrames(0us, buffer_spec)},
        {1ms,  usecsToFrames(20us, buffer_spec)},
        {5ms,  usecsToFrames(70us, buffer_spec)},
        {6ms,  usecsToFrames(20us, buffer_spec)},
        {11ms, usecsToFrames(70us, buffer_spec)},
        //{30ms, usecsToFrames(30us, buffer_spec)},
        //{60ms, usecsToFrames(70us, buffer_spec)},
      };

      static const filter::Automation noise_envelope = {
        {0ms,  0.0},
        {1ms,  1.0},
        {5ms,  0.1},
        {6ms,  1.0},
        {11ms, 0.1},
        {60ms, 0.0}
      };

      volume = std::clamp(volume, 0.0f, 1.0f);
      balance = std::clamp(balance, -1.0f, 1.0f);

      float balance_l = (balance > 0) ? -volume * balance + 1 : volume;
      float balance_r = (balance < 0) ?  volume * balance + 1 : volume;

      // static const filter::Automation final_smooth_kw = {
      //   {0ms,  usecsToFrames(0us, buffer_spec)},
      //   {30ms, usecsToFrames(0us, buffer_spec)},
      //   {60ms, usecsToFrames(200us, buffer_spec)},
      // };

      auto noise_filter =
          filter::std::Noise {noise_gain}
        | filter::std::Smooth {noise_smooth_kw}
        | filter::std::Gain {noise_envelope}
        | filter::std::Mix {osc_buffer}
        | filter::std::Normalize {balance_l, balance_r};

      // apply filter
      noise_filter(buffer);
    }
    return resample(buffer, spec);
  }

  SoundPackage generateClickPackage(const StreamSpec& spec,
                                    float timbre,
                                    float pitch,
                                    float volume,
                                    float balance,
                                    std::size_t package_size)
  {
    std::vector<std::future<ByteBuffer>> results;
    results.reserve(package_size);

    for (std::size_t p = 0; p < package_size; ++p)
      results.push_back( std::async(std::launch::async,
                                    generateClick,
                                    spec, timbre, pitch, volume, balance) );
    SoundPackage package;
    package.reserve(package_size);

    for (auto& result : results)
      package.push_back( result.get() );

    return package;
  }

}//namespace synth
}//namespace audio
