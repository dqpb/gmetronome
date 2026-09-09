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

  void clampSoundParameters(SoundParameters& p)
  {
    p.tone_pitch  = std::clamp(p.tone_pitch,      40.0f, 10000.0f);     // hertz
    p.tone_timbre = std::clamp(p.tone_timbre,      0.0f,     3.0f);     //
    p.tone_detune = std::clamp(p.tone_detune,      0.0f,   100.0f);     // cents
    p.tone_attack = std::clamp(p.tone_attack,      0.0f,    20.0f);     // ms
    clampEnvelopeRampShape(p.tone_attack_shape);
    p.tone_hold   = std::clamp(p.tone_hold,        0.0f,    20.0f);     // ms
    clampEnvelopeHoldShape(p.tone_hold_shape);
    p.tone_decay  = std::clamp(p.tone_decay,       0.0f,    20.0f);     // ms
    clampEnvelopeRampShape(p.tone_decay_shape);

    p.noise_cutoff = std::clamp(p.noise_cutoff,   40.0f, 10000.0f);     // hertz
    p.noise_attack = std::clamp(p.noise_attack,    0.0f,    20.0f);     // ms
    clampEnvelopeRampShape(p.noise_attack_shape);
    p.noise_hold   = std::clamp(p.noise_hold,      0.0f,    20.0f);     // ms
    clampEnvelopeHoldShape(p.noise_hold_shape);
    p.noise_decay  = std::clamp(p.noise_decay,     0.0f,    20.0f);     // ms
    clampEnvelopeRampShape(p.noise_decay_shape);

    p.mix    = std::clamp(p.mix,   -100.0f, 100.0f);   // percent
    p.pan    = std::clamp(p.pan,   -100.0f, 100.0f);   // percent
    p.volume = std::clamp(p.volume,   0.0f, 150.0f);   // percent
  }

  SoundParameters clampSoundParameters(const SoundParameters& params)
  {
    SoundParameters p = params;
    clampSoundParameters(p);
    return p;
  }

  void clampEnvelopeRampShape(EnvelopeRampShape& shape)
  {
    switch (shape) {
    case EnvelopeRampShape::kLinear:
    case EnvelopeRampShape::kCubic:
    case EnvelopeRampShape::kCubicFlipped:
      break;
    default:
      shape = EnvelopeRampShape::kLinear;
      break;
    }
  }

  void clampEnvelopeHoldShape(EnvelopeHoldShape& shape)
  {
    switch (shape) {
    case EnvelopeHoldShape::kKeep:
    case EnvelopeHoldShape::kQuartic:
      break;
    default:
      shape = EnvelopeHoldShape::kKeep;
      break;
    }
  }

  Synthesizer::Synthesizer(const StreamSpec& spec)
    : spec_{SampleFormat::kUnknown, 0, 0}
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

    if (spec == spec_)
      return;

    // rebuild wavetables
    wavetables_.prepare(spec.rate);
    wavetables_.apply();

    // configure filter pipes
    filter::get<1>(tone_pipe_).setWavetable(&wavetables_[kSineTable]);
    filter::get<2>(tone_pipe_).setWavetable(&wavetables_[kTriangleTable]);
    filter::get<3>(tone_pipe_).setWavetable(&wavetables_[kSawtoothTable]);
    filter::get<4>(tone_pipe_).setWavetable(&wavetables_[kSquareTable]);

    // resize audio buffers
    const StreamSpec filter_buffer_spec =
      { filter::kDefaultSampleFormat, spec.rate, 2 };

    noise_buffer_.resize(filter_buffer_spec, kSoundDuration);
    tone_buffer_.resize(filter_buffer_spec, kSoundDuration);

    // prepare filter pipes
    noise_pipe_.prepare(filter_buffer_spec);
    tone_pipe_.prepare(filter_buffer_spec);

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
    if (buffer.spec() != spec_ || buffer.frames() < usecsToFrames(kSoundDuration, spec_))
    {
#ifndef NDEBUG
      std::cerr << "Synthesizer: resizing sound buffer" << std::endl;
#endif
      buffer.resize(spec_, kSoundDuration);
    }

    SoundParameters p = clampSoundParameters(params);

    float gain = volumeToGain(p.volume);

    float tone_gain  = std::cos( (M_PI / 2.0) * (100.0 + p.mix) / 200.0 );
    float noise_gain = std::sin( (M_PI / 2.0) * (100.0 + p.mix) / 200.0 );

    float sine_gain     = tone_gain * std::clamp( 1.0f - std::abs(0.0f - p.tone_timbre), 0.0f, 1.0f);
    float triangle_gain = tone_gain * std::clamp( 1.0f - std::abs(1.0f - p.tone_timbre), 0.0f, 1.0f);
    float sawtooth_gain = tone_gain * std::clamp( 1.0f - std::abs(2.0f - p.tone_timbre), 0.0f, 1.0f);
    float square_gain   = tone_gain * std::clamp( 1.0f - std::abs(3.0f - p.tone_timbre), 0.0f, 1.0f);

    auto tone_envelope = buildEnvelope(
      p.tone_attack,
      p.tone_attack_shape,
      p.tone_hold,
      p.tone_hold_shape,
      p.tone_decay,
      p.tone_decay_shape);

    auto noise_envelope = buildEnvelope(
      p.noise_attack,
      p.noise_attack_shape,
      p.noise_hold,
      p.noise_hold_shape,
      p.noise_decay,
      p.noise_decay_shape);

    filter::std::Wave::Parameters sine_params =
      {
        p.tone_pitch,
        sine_gain,
        0.0f,
        p.tone_detune
      };
    filter::std::Wave::Parameters triangle_params =
      {
        p.tone_pitch,
        triangle_gain,
        0.0f,
        p.tone_detune
      };
    filter::std::Wave::Parameters sawtooth_params =
      {
        p.tone_pitch,
        sawtooth_gain,
        0.0f,
        p.tone_detune
      };
    filter::std::Wave::Parameters square_params =
      {
        p.tone_pitch,
        square_gain,
        0.0f,
        p.tone_detune
      };

    // configure noise pipe
    filter::get<filter::std::Noise>   (noise_pipe_).setGain (noise_gain);
    filter::get<filter::std::Lowpass> (noise_pipe_).setCutoff (p.noise_cutoff);
    filter::get<filter::std::Gain>    (noise_pipe_).setEnvelope (std::move(noise_envelope));

    // apply noise pipe
    noise_pipe_.process(noise_buffer_);

    // configure oscillator pipe
    filter::get<1> /* Wave */ (tone_pipe_).setParameters(sine_params);
    filter::get<2> /* Wave */ (tone_pipe_).setParameters(triangle_params);
    filter::get<3> /* Wave */ (tone_pipe_).setParameters(sawtooth_params);
    filter::get<4> /* Wave */ (tone_pipe_).setParameters(square_params);
    filter::get<5> /* Gain */ (tone_pipe_).setEnvelope(std::move(tone_envelope));
    filter::get<6> /* Mix  */ (tone_pipe_).setBuffer(&noise_buffer_);
    filter::get<6>            (tone_pipe_).setGain(gain);
    filter::get<6>            (tone_pipe_).setPan( p.pan / 100.0f );

    // apply oscillator pipe
    tone_pipe_.process(tone_buffer_);

    // resample from floating point to target format
    resample(tone_buffer_, buffer);
  }

  namespace {

    constexpr float cube(float arg)
    { return arg * arg * arg; }

    constexpr float flip(float arg)
    { return 1.0f - arg; }

    std::function<float(float)> shapeProjection(EnvelopeRampShape shape)
    {
      std::function<float(float)> proj;

      switch (shape) {
      case EnvelopeRampShape::kCubic:
        proj = cube;
        break;
      case EnvelopeRampShape::kCubicFlipped:
        proj = [] (float arg) { return flip(cube(flip(arg))); };
        break;
      case EnvelopeRampShape::kLinear:
        [[fallthrough]];
      default:
        // when switching to C++20 use std::identity
        proj = [] (float arg) { return arg; };
        break;
      };

      return proj;
    }

    std::function<float(float)> shapeProjection(EnvelopeHoldShape shape)
    {
      std::function<float(float)> proj;

      switch (shape) {
      case EnvelopeHoldShape::kQuartic:
        proj = [] (float arg) { return std::pow(2.0f * arg - 1.0f, 4.0f); };
        break;
      case EnvelopeHoldShape::kKeep:
        [[fallthrough]];
      default:
        proj = [] (float arg) { return 1.0f; };
        break;
      };

      return proj;
    }

  }//unnnamed namespace

  filter::Automation
  Synthesizer::buildEnvelope(float attack, EnvelopeRampShape attack_shape,
                             float hold, EnvelopeHoldShape hold_shape,
                             float decay, EnvelopeRampShape decay_shape) const
  {
    using milliseconds_dbl = std::chrono::duration<double, std::milli>;

    const auto attack_tm = milliseconds_dbl(attack);
    const auto hold_tm = attack_tm + milliseconds_dbl(hold);
    const auto decay_tm = hold_tm + milliseconds_dbl(decay);

    const auto attack_step_tm = attack_tm / 5.0;
    const auto hold_step_tm = (hold_tm - attack_tm) / 5.0;
    const auto decay_step_tm = (decay_tm - hold_tm) / 5.0;

    std::function<float(float)> attack_proj = shapeProjection(attack_shape);
    std::function<float(float)> hold_proj = shapeProjection(hold_shape);
    std::function<float(float)> decay_proj = shapeProjection(decay_shape);

    filter::Automation envelope {{0ms, 0.0f}};

    if (attack_tm == 0ms && hold_tm == 0ms && decay_tm == 0ms )
      return envelope;

    if (attack_tm > 0ms)
    {
      envelope.append({
          { 1.0 * attack_step_tm, attack_proj(0.2f) },
          { 2.0 * attack_step_tm, attack_proj(0.4f) },
          { 3.0 * attack_step_tm, attack_proj(0.6f) },
          { 4.0 * attack_step_tm, attack_proj(0.8f) },

          { attack_tm, 1.0f }
        });
    }
    else envelope.append({{attack_tm, 1.0f}});

    if (hold_tm > attack_tm)
    {
      envelope.append({
          { attack_tm + 1.0 * hold_step_tm, hold_proj(0.2f) },
          { attack_tm + 2.0 * hold_step_tm, hold_proj(0.4f) },
          { attack_tm + 3.0 * hold_step_tm, hold_proj(0.6f) },
          { attack_tm + 4.0 * hold_step_tm, hold_proj(0.8f) },

          { hold_tm, 1.0f }
        });
    }

    if (decay_tm > hold_tm)
    {
      envelope.append({
          { hold_tm + 1.0 * decay_step_tm, decay_proj( flip(0.2f) ) },
          { hold_tm + 2.0 * decay_step_tm, decay_proj( flip(0.4f) ) },
          { hold_tm + 3.0 * decay_step_tm, decay_proj( flip(0.6f) ) },
          { hold_tm + 4.0 * decay_step_tm, decay_proj( flip(0.8f) ) },

          { decay_tm, 0.0f }
        });
    }
    else envelope.append({{decay_tm, 0.0f}});

    return envelope;
  }

}//namespace audio
