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
#include <functional>
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

    // rebuild wavetables
    wavetables_.prepare(spec.rate);
    wavetables_.apply();

    // configure filter pipes
    filter::get<1>(osc_pipe_).setWavetable(&wavetables_[kSineTable]);
    filter::get<2>(osc_pipe_).setWavetable(&wavetables_[kTriangleTable]);
    filter::get<3>(osc_pipe_).setWavetable(&wavetables_[kSawtoothTable]);
    filter::get<4>(osc_pipe_).setWavetable(&wavetables_[kSquareTable]);

    // resize audio buffers
    const StreamSpec filter_buffer_spec =
      { filter::kDefaultSampleFormat, spec.rate, 2 };

    noise_buffer_.resize(filter_buffer_spec, kSoundDuration);
    osc_buffer_.resize(filter_buffer_spec, kSoundDuration);

    // prepare filter pipes
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

    float osc_pitch          = std::clamp(params.tone_pitch, 40.0f, 10000.0f);
    float osc_timbre         = std::clamp(params.tone_timbre, 0.0f, 3.0f);
    float osc_detune         = std::clamp(params.tone_detune, 0.0f, 100.0f);
    float osc_attack         = std::clamp(params.tone_attack, 0.0f, 1.0f);
    auto  osc_attack_shape   = params.tone_attack_shape;
    float osc_decay          = std::clamp(params.tone_decay, 0.0f, 1.0f);
    auto  osc_decay_shape    = params.tone_decay_shape;

    float noise_cutoff       = std::clamp(params.percussion_cutoff, 40.0f, 10000.0f);
    float noise_clap         = std::clamp(params.percussion_clap, 0.0f, 1.0f);
    float noise_attack       = std::clamp(params.percussion_attack, 0.0f, 1.0f);
    auto  noise_attack_shape = params.percussion_attack_shape;
    float noise_decay        = std::clamp(params.percussion_decay, 0.0f, 1.0f);
    auto  noise_decay_shape  = params.percussion_decay_shape;

    float mix      = std::clamp(params.mix, -100.0f, 100.0f);
    float balance  = std::clamp(params.balance, -100.0f, 100.0f);
    float volume   = std::clamp(params.volume, 0.0f, 100.0f);

    float balance_vol_l = (balance > 0.0) ? 100.0 - balance : 100.0;
    float balance_vol_r = (balance < 0.0) ? 100.0 + balance : 100.0;

    float balance_l = volumeToAmplitude( balance_vol_l, VolumeMapping::kLinear );
    float balance_r = volumeToAmplitude( balance_vol_r, VolumeMapping::kLinear );

    float volume_l = volumeToAmplitude(volume) * balance_l;
    float volume_r = volumeToAmplitude(volume) * balance_r;

    Decibel noise_gain = volumeToDecibel( (mix + 100.0) / 2.0, VolumeMapping::kQuadratic );
    Decibel osc_gain   = volumeToDecibel( (100.0 - mix) / 2.0, VolumeMapping::kQuadratic );

    Decibel sine_gain      = osc_gain - 18_dB * std::abs(0.0f - osc_timbre);
    Decibel triangle_gain  = osc_gain - 18_dB * std::abs(1.0f - osc_timbre);
    Decibel sawtooth_gain  = osc_gain - 18_dB * std::abs(2.0f - osc_timbre);
    Decibel square_gain    = osc_gain - 18_dB * std::abs(3.0f - osc_timbre);

    auto osc_envelope
      = buildEnvelope(osc_attack, osc_attack_shape, osc_decay, osc_decay_shape, 0.0);

    auto noise_envelope
      = buildEnvelope(noise_attack, noise_attack_shape,
                      noise_decay, noise_decay_shape, noise_clap);

    filter::std::Wave::Parameters sine_params =
      {
        osc_pitch,
        float(sine_gain.amplitude()),
        float(M_PI/2.0),
        osc_detune
      };
    filter::std::Wave::Parameters triangle_params =
      {
        osc_pitch,
        float(triangle_gain.amplitude()),
        0.0f,
        osc_detune
      };
    filter::std::Wave::Parameters sawtooth_params =
      {
        osc_pitch,
        float(sawtooth_gain.amplitude()),
        float(M_PI),
        osc_detune
      };
    filter::std::Wave::Parameters square_params =
      {
        osc_pitch,
        float(square_gain.amplitude()),
        0.0f,
        osc_detune
      };

    // configure noise pipe
    filter::get<filter::std::Noise>   (noise_pipe_).setLevel (noise_gain);
    filter::get<filter::std::Lowpass> (noise_pipe_).setCutoff (noise_cutoff);
    filter::get<filter::std::Gain>    (noise_pipe_).setEnvelope (std::move(noise_envelope));

    // apply noise pipe
    noise_pipe_.process(noise_buffer_);

    // configure oscillator pipe
    filter::get<1> /* Wave */           (osc_pipe_).setParameters(sine_params);
    filter::get<2> /* Wave */           (osc_pipe_).setParameters(triangle_params);
    filter::get<3> /* Wave */           (osc_pipe_).setParameters(sawtooth_params);
    filter::get<4> /* Wave */           (osc_pipe_).setParameters(square_params);
    filter::get<filter::std::Gain>      (osc_pipe_).setEnvelope(std::move(osc_envelope));
    filter::get<filter::std::Mix>       (osc_pipe_).setBuffer(&noise_buffer_);
    filter::get<filter::std::Normalize> (osc_pipe_).setAmplitude(volume_l, volume_r);

    // apply oscillator pipe
    osc_pipe_.process(osc_buffer_);

    // resample from floating point to target format
    resample(osc_buffer_, buffer);
  }

  std::function<float(float)> shapeProjection(EnvelopeShape shape)
  {
    std::function<float(float)> proj;

    switch (shape) {
    case EnvelopeShape::kCubic:
      proj = [] (float arg) { return arg * arg * arg; };
      break;
    case EnvelopeShape::kCubicRoot:
      proj = std::cbrtf;
      break;
    case EnvelopeShape::kLinear:
      [[fallthrough]];
    default:
      // when switching to C++20 use std::identity
      proj = [] (float arg) { return arg; };
      break;
    };

    return proj;
  }

  filter::Automation
  Synthesizer::buildEnvelope(float attack, EnvelopeShape attack_shape,
                             float decay, EnvelopeShape decay_shape, float clap) const
  {
    using filter::seconds_dbl;

    filter::Automation envelope;

    constexpr seconds_dbl min_attack_tm   = 1ms;
    constexpr seconds_dbl max_attack_tm   = 25ms;
    constexpr seconds_dbl delta_attack_tm = max_attack_tm - min_attack_tm;
    const     seconds_dbl attack_tm       = min_attack_tm + delta_attack_tm * attack;

    const     seconds_dbl min_clap_tm     = attack_tm;
    const     seconds_dbl max_clap_tm     = min_clap_tm + 20ms;
    const     seconds_dbl delta_clap_tm   = max_clap_tm - min_clap_tm;
    const     seconds_dbl clap_tm         = min_clap_tm + delta_clap_tm * clap;

    const     seconds_dbl min_decay_tm    = clap_tm + 1ms;
    constexpr seconds_dbl max_decay_tm    = kSoundDuration;
    const     seconds_dbl delta_decay_tm  = max_decay_tm - min_decay_tm;
    const     seconds_dbl decay_tm        = min_decay_tm + delta_decay_tm * decay;

    const seconds_dbl attack_step_tm = attack_tm / 5.0;
    const seconds_dbl clap_step_tm = (clap_tm - attack_tm) / 5.0;
    const seconds_dbl decay_step_tm = (decay_tm - clap_tm) / 5.0;

    std::function<float(float)> attack_proj = shapeProjection(attack_shape);
    std::function<float(float)> clap_proj = [] (float arg) { return std::pow(arg, 1.0f / 6.0f); };
    // std::function<float(float)> clap_proj = shapeProjection(EnvelopeShape::kCubicRoot);
    std::function<float(float)> decay_proj = shapeProjection(decay_shape);

    envelope.append({
        { 0.0 * attack_step_tm, 0.0f },

        { 1.0 * attack_step_tm, attack_proj(0.2f) },
        { 2.0 * attack_step_tm, attack_proj(0.4f) },
        { 3.0 * attack_step_tm, attack_proj(0.6f) },
        { 4.0 * attack_step_tm, attack_proj(0.8f) }
      });

    if (clap_tm > attack_tm)
    {
      envelope.append({
          { attack_tm, 1.0f },

          { attack_tm + 1.0 * clap_step_tm, 1.0f - clap_proj(0.2f) },
          { attack_tm + 2.0 * clap_step_tm, 1.0f - clap_proj(0.4f) },
          { attack_tm + 3.0 * clap_step_tm, 1.0f - clap_proj(0.6f) },
          { attack_tm + 4.0 * clap_step_tm, 1.0f - clap_proj(0.8f) },
        });
    }
    envelope.append({
        { clap_tm, 1.0f },

        { clap_tm + 1.0 * decay_step_tm, 1.0f - decay_proj(0.2f) },
        { clap_tm + 2.0 * decay_step_tm, 1.0f - decay_proj(0.4f) },
        { clap_tm + 3.0 * decay_step_tm, 1.0f - decay_proj(0.6f) },
        { clap_tm + 4.0 * decay_step_tm, 1.0f - decay_proj(0.8f) },

        { decay_tm, 0.0f }
      });

    return envelope;
  }

}//namespace audio
