/*
 * Copyright (C) 2021-2023 The GMetronome Team
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
# include <config.h>
#endif

#include "Generator.h"
#include <cmath>
#include <ratio>
#include <numeric>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  namespace {
    constexpr microseconds kMaxChunkDuration = 80ms;
    constexpr microseconds kAvgChunkDuration = 50ms;
    constexpr microseconds kFillBufferDuration = 200ms;

    // not implemented yet
    // constexpr microseconds kDrainBufferDuration = 50ms;

  }//unnamed namespace

  // FillBufferGenerator
  //
  void FillBufferGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_frames_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                                 ctrl.sound(kAccentOff).frames());

    avg_chunk_frames_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());

    double percentage = (frames_total_ > 0) ? 100.0 * frames_done_ / frames_total_ : 0;

    frames_total_ = usecsToFrames(kFillBufferDuration, ctrl.spec());
    frames_done_ = frames_total_ / 100.0 * percentage;
  }

  void FillBufferGenerator::enter(BeatStreamController& ctrl)
  {
    frames_done_ = 0;
  }

  void FillBufferGenerator::leave(BeatStreamController& ctrl)
  {
    // nothing
  }

  void FillBufferGenerator::cycle(BeatStreamController& ctrl,
                                  const void*& data, size_t& bytes)
  {
    size_t frames_left = frames_total_ - frames_done_;
    size_t frames_chunk = 0;

    if (frames_left <= max_chunk_frames_)
      frames_chunk = frames_left;
    else
      frames_chunk = frames_left / std::lround( (double) frames_left / avg_chunk_frames_ );

    data = ctrl.sound(kAccentOff).data();
    bytes = frames_chunk * frameSize(ctrl.spec());

    frames_done_ += frames_chunk;

    if (frames_done_ >= frames_total_)
      switchGenerator(ctrl, kPreCountGenerator);
  }

  void FillBufferGenerator::updateStatus(BeatStreamController& ctrl, StreamStatus& status)
  {
    size_t frames_left = frames_total_ - frames_done_;

    physics::seconds_dbl time_left {(double)frames_left / ctrl.spec().rate};

    status.position = - ctrl.countIn() - ctrl.tempo() * time_left.count() / 60.0;
    status.tempo = ctrl.tempo();
    status.acceleration = 0.0;
    status.accent = -ctrl.countIn() - 1;
    status.next_accent_delay = std::chrono::duration_cast<microseconds>(time_left);
    status.generator = kFillBufferGenerator;
  }

  // PreCountGenerator
  //
  void PreCountGenerator::onTempoChanged(BeatStreamController& ctrl, TempoMode old_mode)
  {
    auto& k = ctrl.kinematics();

    switch (ctrl.mode()) {
    case TempoMode::kConstant:
      k.setTempo(ctrl.tempo());
      break;
    case TempoMode::kSync:
      k.synchronize(ctrl.syncBeats(), ctrl.syncTempo(), ctrl.syncTime());
      break;
    case TempoMode::kContinuous:
      [[fallthrough]];
    case TempoMode::kStepwise:
      [[fallthrough]];
    default:
      return;
      break;
    };

    updateFramesLeft(ctrl);
  }

  void PreCountGenerator::onCountInChanged(BeatStreamController& ctrl)
  {
    // nothing
  }

  void PreCountGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_frames_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                                 ctrl.sound(kAccentOff).frames());

    avg_chunk_frames_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());

    updateFramesLeft(ctrl);
  }

  void PreCountGenerator::enter(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    k.reset();

    if (ctrl.countIn() > 0)
    {
      k.setTempo(ctrl.tempo());
      k.setBeats(100);
      accent_point_ = true;
      updateFramesLeft(ctrl);
    }
    else
    {
      switchGenerator(ctrl, kRegularGenerator); // skip this generator
    }
  }

  void PreCountGenerator::leave(BeatStreamController& ctrl)
  { }

  void PreCountGenerator::cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes)
  {
    auto& k = ctrl.kinematics();

    size_t frames_chunk = 0;
    if (accent_point_) // play sound
    {
      const auto& sound_buffer = ctrl.sound(kAccentMid);

      frames_chunk = std::min(sound_buffer.frames(), frames_left_);
      data = sound_buffer.data();
      bytes = frames_chunk * frameSize(ctrl.spec());
    }
    else // play silence
    {
      const auto& sound_buffer = ctrl.sound(kAccentOff);

      if (frames_left_ <= max_chunk_frames_)
        frames_chunk = frames_left_;
      else
        frames_chunk = frames_left_ / std::lround( (double) frames_left_ / avg_chunk_frames_ );

      frames_chunk = std::min(sound_buffer.frames(), frames_chunk);
      data = sound_buffer.data();
      bytes = frames_chunk * frameSize(ctrl.spec());
    }

    // update kinematice, frames_left_, ...
    step(ctrl, frames_chunk);

    if (accent_point_ && std::round(k.position()) >= ctrl.countIn())
      switchGenerator(ctrl, kRegularGenerator);
  }

  void PreCountGenerator::updateFramesLeft(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    // compensate for rounding errors
    double beat_position = accent_point_ ? std::round(k.position()) : std::floor(k.position());

    double next_beat_position = (beat_position + 1.0); // in beat units
    double distance = next_beat_position - k.position();

    physics::seconds_dbl arrival_time = k.arrival(distance);
    frames_left_ = std::lround(ctrl.spec().rate * arrival_time.count());
  }

  void PreCountGenerator::step(BeatStreamController& ctrl, size_t frames_chunk)
  {
    auto& k = ctrl.kinematics();

    k.step(framesToUsecs(frames_chunk, ctrl.spec()));

    assert(frames_left_ >= frames_chunk);
    frames_left_ -= frames_chunk;

    if (frames_left_ == 0)
    {
      accent_point_ = true;
      updateFramesLeft(ctrl);
    }
    else
    {
      accent_point_ = false;
    }
  }

  void PreCountGenerator::updateStatus(BeatStreamController& ctrl, StreamStatus& status)
  {
    auto& k = ctrl.kinematics();

    status.position     = -ctrl.countIn() + k.position();
    status.tempo        = k.tempo();
    status.mode         = k.isSynchronizing() ? TempoMode::kSync : TempoMode::kConstant;
    status.acceleration = k.acceleration();
    status.hold         = 0;
    status.accent       = -ctrl.countIn() +
      (accent_point_ ? std::round(k.position()) : std::floor(k.position()));

    const double kMicrosecondsFramesRatio = (double) std::micro::den / ctrl.spec().rate;

    status.next_accent_delay
      = microseconds((microseconds::rep) (frames_left_ * kMicrosecondsFramesRatio));

    status.generator = kPreCountGenerator;
  }

  // RegularGenerator
  //
  void RegularGenerator::onTempoChanged(BeatStreamController& ctrl, TempoMode old_mode)
  {
    auto& k = ctrl.kinematics();

    switch (ctrl.mode()) {
    case TempoMode::kConstant:
      k.setTempo(ctrl.tempo());
      break;

    case TempoMode::kContinuous:
      k.accelerate(ctrl.acceleration(), ctrl.target());
      break;

    case TempoMode::kStepwise:
      if (old_mode == TempoMode::kContinuous)
        k.stopAcceleration();
      else if (old_mode == TempoMode::kSync)
        k.stopSynchronization();

      if (hold_ <= 0) // initialize hold
        resetStepwise(ctrl);

      recomputeStepwise(ctrl);
      break;

    case TempoMode::kSync:
      k.synchronize(ctrl.syncBeats(), ctrl.syncTempo(), ctrl.syncTime());
      break;
    };

    updateFramesLeft(ctrl);
  }

  void RegularGenerator::onMeterChanged(BeatStreamController& ctrl,
                                        const Meter& old_meter, bool enabled_changed)
  {
    auto& k = ctrl.kinematics();
    const Meter& meter = ctrl.meter();

    // play the accent pattern from the beginning, when accentuation was enabled
    if (bool rollover = enabled_changed && ctrl.isMeterEnabled(); rollover)
      k.setBeats(meter.beats(), physics::BeatKinematics::PositionMode::kRollover);
    else
      k.setBeats(meter.beats());

    // If accent_point_ == true (i.e. we are about to play an accent), we check
    // if there is a matching accent in the new meter and set the current accent
    // accordingly
    bool accent_match = (accent_ * meter.division()) % old_meter.division() == 0;
    if (accent_point_ && accent_match)
    {
      accent_ = std::fmod(std::round(k.position() * meter.division()),
                          meter.division() * meter.beats());
    }
    else
    {
      accent_ = std::trunc(k.position() * meter.division());
      accent_point_ = false;
    }

    if (ctrl.mode() == TempoMode::kStepwise)
      recomputeStepwise(ctrl);

    updateFramesLeft(ctrl);
  }

  void RegularGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_frames_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                                 ctrl.sound(kAccentOff).frames());

    avg_chunk_frames_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());

    updateFramesLeft(ctrl);
  }

  void RegularGenerator::enter(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    k.setBeats(ctrl.meter().beats(), physics::BeatKinematics::PositionMode::kZero);

    switch (ctrl.mode()) {
    case TempoMode::kContinuous:
      k.setTempo(ctrl.tempo());
      if (ctrl.target() != ctrl.tempo())
        k.accelerate(ctrl.acceleration(), ctrl.target());
      break;

    case TempoMode::kStepwise:
    case TempoMode::kConstant:
      k.setTempo(ctrl.tempo());
      break;

    case TempoMode::kSync:
      [[fallthrough]];
    default:
      // nothing
      break;
    };

    resetStepwise(ctrl);

    accent_ = 0;
    accent_point_ = true;
    updateFramesLeft(ctrl);
  }

  void RegularGenerator::leave(BeatStreamController& ctrl)
  { }

  void RegularGenerator::cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes)
  {
    const Meter& meter = ctrl.meter();
    const AccentPattern& accents = meter.accents();

    size_t frames_chunk = 0;
    if (accent_point_) // play sound
    {
      const auto& sound_buffer = ctrl.sound(accents[accent_]);

      frames_chunk = std::min(sound_buffer.frames(), frames_left_);

      data = sound_buffer.data();
      bytes = frames_chunk * frameSize(ctrl.spec());
    }
    else // play silence
    {
      const auto& sound_buffer = ctrl.sound(kAccentOff);

      if (frames_left_ <= max_chunk_frames_)
        frames_chunk = frames_left_;
      else
        frames_chunk = frames_left_ / std::lround( (double) frames_left_ / avg_chunk_frames_ );

      frames_chunk = std::min(sound_buffer.frames(), frames_chunk);

      data = sound_buffer.data();
      bytes = frames_chunk * frameSize(ctrl.spec());
    }

    // update kinematics, frames_left_, ...
    step(ctrl, frames_chunk);
  }

  void RegularGenerator::updateStatus(BeatStreamController& ctrl, StreamStatus& status)
  {
    auto& k = ctrl.kinematics();

    status.position     = k.position();
    status.tempo        = k.tempo();
    status.mode         = effectiveMode(ctrl);
    status.acceleration = k.acceleration();
    status.hold         = hold_;
    status.accent       = accent_;

    const double kMicrosecondsFramesRatio = (double) std::micro::den / ctrl.spec().rate;

    status.next_accent_delay
      = microseconds((microseconds::rep) (frames_left_ * kMicrosecondsFramesRatio));

    status.generator = kRegularGenerator;
  }

  void RegularGenerator::updateFramesLeft(BeatStreamController& ctrl)
  {
    const auto& k = ctrl.kinematics();
    const Meter& meter = ctrl.meter();

    double accent_position = 0.0; // in accent units
    if (accent_point_)
      accent_position = std::round(k.position() * meter.division());
    else
      accent_position = std::floor(k.position() * meter.division());

    double next_accent_position = (accent_position + 1.0) / meter.division(); // in beat units
    double distance = next_accent_position - k.position();

    physics::seconds_dbl arrival_time = k.arrival(distance);
    frames_left_ = std::lround(ctrl.spec().rate * arrival_time.count());
  }

  void RegularGenerator::step(BeatStreamController& ctrl, size_t frames_chunk)
  {
    auto& k = ctrl.kinematics();
    const Meter& meter = ctrl.meter();
    const AccentPattern& accents = meter.accents();

    k.step(framesToUsecs(frames_chunk, ctrl.spec()));

    assert(frames_left_ >= frames_chunk);
    frames_left_ -= frames_chunk;

    if (frames_left_ == 0)
    {
      accent_ = (accent_ + 1) % accents.size();
      accent_point_ = true;

      // step hold value on beat position
      if (accent_ % meter.division() == 0)
      {
        hold_ -= 1;
        if (hold_ <= 0)
        {
          if (ctrl.mode() == TempoMode::kStepwise)
            accelerateStepwise(ctrl);

          hold_ = ctrl.hold();
        }
      }

      updateFramesLeft(ctrl);
    }
    else
    {
      accent_point_ = false;
    }
  }

  void RegularGenerator::recomputeStepwise(BeatStreamController& ctrl)
  {
    int meter_beats    = ctrl.meter().beats();
    int meter_division = ctrl.meter().division();

    if (int gcd = std::gcd(ctrl.hold(), meter_beats); gcd > 1)
    {
      int beat = accent_ / meter_division;                // current beat
      int rem_beat = (meter_beats - beat);                // remaining beats in measure
      int offset = std::remainder(rem_beat - hold_, gcd); // displacement (mod gcd)

      hold_ = aux::math::modulo(hold_ + offset, ctrl.hold());

      if (hold_ == 0)
        hold_ = ctrl.hold();
    }
    else
      hold_ = std::min(hold_, ctrl.hold());
  }

  void RegularGenerator::resetStepwise(BeatStreamController& ctrl)
  {
    hold_ = ctrl.hold();
  }

  void RegularGenerator::accelerateStepwise(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    double tempo = k.tempo();
    double tempo_diff = ctrl.target() - tempo;

    if (std::abs(ctrl.step()) <= std::abs(tempo_diff))
    {
      double step = std::copysign(ctrl.step() , tempo_diff);
      k.setTempo(tempo + step);
    }
    else
    {
      k.setTempo(ctrl.target());
    }
  }

  TempoMode RegularGenerator::effectiveMode(const BeatStreamController& ctrl) const
  {
    const auto& k = ctrl.kinematics();
    TempoMode m = TempoMode::kConstant;

    switch (ctrl.mode()) {
    case TempoMode::kConstant:
      // nothing
      break;

    case TempoMode::kSync:
      if (k.isSynchronizing())
        m = TempoMode::kSync;
      break;

    case TempoMode::kContinuous:
      if (k.isAccelerating())
        m = TempoMode::kContinuous;
      break;

    case TempoMode::kStepwise:

      break;
    };
    return m;
  }


  // DrainGenerator
  //
  void DrainBufferGenerator::cycle(BeatStreamController& ctrl,
                                   const void*& data, size_t& bytes)
  {
    // not implemented yet
  }

}//namespace audio
