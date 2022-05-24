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

#include "Synthesizer.h"

#include <algorithm>
#include <cmath>
#include <cassert>

#ifndef NDEBUG
#  include <iostream>
#endif

namespace audio {

  Synthesizer::Synthesizer(const StreamSpec& spec)
  {
    wavetables_.insert(kSineTable, std::make_shared<SineRecipe>());
    wavetables_.insert(kTriangleTable, std::make_shared<TriangleRecipe>());
    wavetables_.insert(kSawtoothTable, std::make_shared<SawtoothRecipe>());
    wavetables_.insert(kSquareTable, std::make_shared<SquareRecipe>());

    prepare(spec);
  }

  void Synthesizer::prepare(const StreamSpec& spec)
  {
    assert(spec.rate > 0);
    assert(spec.channels == 2);

    wavetables_.prepare(spec.rate);
    wavetables_.apply();

    const StreamSpec filter_buffer_spec =
      { filter::kDefaultSampleFormat, spec.rate, 2 };

    noise_buffer_.resize(filter_buffer_spec, kSoundDuration);
    osc_buffer_.resize(filter_buffer_spec, kSoundDuration);

    noise_pipe_.prepare(filter_buffer_spec);
    osc_pipe_.prepare(filter_buffer_spec);

    spec_ = spec;
  }

  ByteBuffer Synthesizer::create(const SoundParameters& params)
  {
    ByteBuffer buffer(spec_, kSoundDuration);
    update(buffer, params);
    return buffer;
  }

  void Synthesizer::update(ByteBuffer& buffer, const SoundParameters& params)
  {
    assert(!std::isnan(params.tone_timbre));
    assert(!std::isnan(params.tone_pitch));
    //...

    if (buffer.spec() != spec_ || buffer.frames() < usecsToFrames(kSoundDuration, spec_))
    {
#ifndef NDEBUG
      std::cerr << "Synthesizer: resizing sound buffer" << std::endl;
#endif
      buffer.resize(spec_, kSoundDuration);
    }

    float tone_pitch         = std::clamp(params.tone_pitch, 40.0f, 10000.0f);
    float tone_timbre        = std::clamp(params.tone_timbre, 0.0f, 3.0f);
    float tone_detune        = std::clamp(params.tone_detune, 0.0f, 100.0f);
    float tone_punch         = std::clamp(params.tone_punch, 0.0f, 1.0f);
    float tone_decay         = std::clamp(params.tone_decay, 0.0f, 1.0f);
    float percussion_cutoff  = std::clamp(params.percussion_cutoff, 40.0f, 10000.0f);
    bool  percussion_clap    = params.percussion_clap;
    float percussion_punch   = std::clamp(params.percussion_punch, 0.0f, 1.0f);
    float percussion_decay   = std::clamp(params.percussion_decay, 0.0f, 1.0f);
    float mix                = std::clamp(params.mix, -100.0f, 100.0f);
    float balance            = std::clamp(params.balance, -100.0f, 100.0f);
    float volume             = std::clamp(params.volume, 0.0f, 100.0f);

    float balance_vol_l = (balance > 0.0) ? 100.0 - balance : 100.0;
    float balance_vol_r = (balance < 0.0) ? 100.0 + balance : 100.0;

    float balance_l = volumeToAmplitude( balance_vol_l, VolumeMapping::kLinear );
    float balance_r = volumeToAmplitude( balance_vol_r, VolumeMapping::kLinear );

    float volume_l = volumeToAmplitude(volume) * balance_l;
    float volume_r = volumeToAmplitude(volume) * balance_r;

    Decibel noise_gain    = volumeToDecibel( (mix + 100.0) / 2.0, VolumeMapping::kQuadratic );
    Decibel osc_gain      = volumeToDecibel( (100.0 - mix) / 2.0, VolumeMapping::kQuadratic );

    Decibel sine_gain      = osc_gain - 18_dB * std::abs(0.0f - tone_timbre);
    Decibel triangle_gain  = osc_gain - 18_dB * std::abs(1.0f - tone_timbre);
    Decibel sawtooth_gain  = osc_gain - 18_dB * std::abs(2.0f - tone_timbre);
    Decibel square_gain    = osc_gain - 18_dB * std::abs(3.0f - tone_timbre);

    auto [osc_envelope, osc_full_gain_time, osc_full_decay_time]
      = buildEnvelope(tone_punch, tone_decay, false);

    auto [noise_envelope, noise_full_gain_time, noise_full_decay_time]
      = buildEnvelope(percussion_punch, percussion_decay, percussion_clap);

    // configure noise pipe
    filter::get<filter::std::Noise>   (noise_pipe_).setLevel (noise_gain);
    filter::get<filter::std::Lowpass> (noise_pipe_).setCutoff (percussion_cutoff);
    filter::get<filter::std::Gain>    (noise_pipe_).setEnvelope (std::move(noise_envelope));

    // apply noise pipe
    noise_pipe_.process(noise_buffer_);

    auto osc_filter =
      filter::std::Zero {}
    | filter::std::Wave {wavetables_[kSineTable],     tone_pitch, sine_gain,     float(M_PI/2.0),  tone_detune}
    | filter::std::Wave {wavetables_[kTriangleTable], tone_pitch, triangle_gain, 0.0f,  tone_detune}
    | filter::std::Wave {wavetables_[kSawtoothTable], tone_pitch, sawtooth_gain, float(M_PI), tone_detune}
    | filter::std::Wave {wavetables_[kSquareTable],   tone_pitch, square_gain,   0.0f,  tone_detune}
    | filter::std::Gain {osc_envelope}
    | filter::std::Mix {&noise_buffer_}
    | filter::std::Normalize {volume_l, volume_r};

    // apply filter
    osc_filter.process(osc_buffer_);

    resample(osc_buffer_, buffer);
  }

  std::tuple<filter::Automation, microseconds, microseconds>
  Synthesizer::buildEnvelope(float punch, float decay, bool clap) const
  {
    constexpr microseconds min_full_gain_time = 1ms;
    constexpr microseconds max_full_gain_time = 30ms;
    constexpr microseconds delta_full_gain_time = max_full_gain_time - min_full_gain_time;

    const microseconds full_gain_time = min_full_gain_time
      + microseconds { int ( (1.0f - punch) * delta_full_gain_time.count() ) };

    microseconds delta_attack = full_gain_time / 5;

    const microseconds min_full_decay_time = full_gain_time + 5ms;
    const microseconds max_full_decay_time = kSoundDuration;
    const microseconds delta_full_decay_time = max_full_decay_time - min_full_decay_time;

    const microseconds full_decay_time = min_full_decay_time
      + microseconds { int ( (1.0f - decay) * delta_full_decay_time.count() ) };

    const microseconds  delta_decay = (full_decay_time - full_gain_time) / 5;

    filter::Automation envelope;

    float drop = 1.5f;

    if (clap)
    {
      delta_attack /= 2;

      envelope.append({
          {  0 * delta_attack, 0.0f},

          {  1 * delta_attack, std::pow(0.2f, (punch + 1.0f) * drop)},
          {  2 * delta_attack, std::pow(0.4f, (punch + 1.0f) * drop)},
          {  3 * delta_attack, std::pow(0.6f, (punch + 1.0f) * drop)},
          {  4 * delta_attack, std::pow(0.8f, (punch + 1.0f) * drop)},

          {  5 * delta_attack, 1.0f },

          {  6 * delta_attack, std::pow(0.2f, (punch + 1.0f) * drop)},
          {  7 * delta_attack, std::pow(0.4f, (punch + 1.0f) * drop)},
          {  8 * delta_attack, std::pow(0.6f, (punch + 1.0f) * drop)},
          {  9 * delta_attack, std::pow(0.8f, (punch + 1.0f) * drop)},

          { 10 * delta_attack, 1.0f },

          {  full_gain_time + 1 * delta_decay, std::pow(0.8f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 2 * delta_decay, std::pow(0.6f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 3 * delta_decay, std::pow(0.4f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 4 * delta_decay, std::pow(0.2f, (decay + punch + 1.0f) * drop)},

          {  full_gain_time + 5 * delta_decay, 0.0f }
        });
    }
    else
    {
      envelope.append({
          {  0 * delta_attack, 0.0f},

          {  1 * delta_attack, std::pow(0.2f, (punch + 1.0f) * drop)},
          {  2 * delta_attack, std::pow(0.4f, (punch + 1.0f) * drop)},
          {  3 * delta_attack, std::pow(0.6f, (punch + 1.0f) * drop)},
          {  4 * delta_attack, std::pow(0.8f, (punch + 1.0f) * drop)},

          {  5 * delta_attack, 1.0f },

          {  full_gain_time + 1 * delta_decay, std::pow(0.8f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 2 * delta_decay, std::pow(0.6f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 3 * delta_decay, std::pow(0.4f, (decay + punch + 1.0f) * drop)},
          {  full_gain_time + 4 * delta_decay, std::pow(0.2f, (decay + punch + 1.0f) * drop)},

          {  full_gain_time + 5 * delta_decay, 0.0f }
        });
    }

    return {std::move(envelope), std::move(full_gain_time), std::move(full_decay_time)};
  }

}//namespace audio
