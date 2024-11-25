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
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  namespace {
    constexpr microseconds kMaxChunkDuration = 80ms;
    constexpr microseconds kAvgChunkDuration = 50ms;
    constexpr microseconds kFillBufferDuration = 200ms;
    constexpr microseconds kKinematicsSyncTime = 1000ms;

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
  { }

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

    status.position = - ctrl.tempo() * time_left.count() / 60.0;
    status.tempo = ctrl.tempo();
    status.acceleration = 0.0;
    status.next_accent = 0;
    status.next_accent_delay = std::chrono::duration_cast<microseconds>(time_left);
    status.state = kFillBufferGenerator;
  }

  // PreCountGenerator (not implemented yet)
  //
  void PreCountGenerator::enter(BeatStreamController& ctrl)
  {
    switchGenerator(ctrl, kRegularGenerator); // skip this generator
  }

  void PreCountGenerator::leave(BeatStreamController& ctrl)
  { }

  // RegularGenerator
  //
  void RegularGenerator::onTempoChanged(BeatStreamController& ctrl)
  {
    k_.setTempo(ctrl.tempo());
    updateFramesLeft(ctrl);
  }

  void RegularGenerator::configureAcceleration(BeatStreamController& ctrl)
  {
    switch (ctrl.accelerationMode()) {
    case AccelerationMode::kContinuous:
      k_.accelerate(ctrl.acceleration(), ctrl.targetTempo());
      hold_pos_ = 0;
      break;
    case AccelerationMode::kStepwise:
      if (k_.acceleration() != 0.0)
        k_.accelerate(0.0, 0.0);
      if (hold_pos_ > 0)
        hold_pos_ = hold_pos_ % ctrl.hold();
      break;
    case AccelerationMode::kNoAcceleration:
      if (k_.acceleration() != 0.0)
        k_.accelerate(0.0, 0.0);
      hold_pos_ = 0;
      break;
    };
  }

  void RegularGenerator::onAccelerationChanged(BeatStreamController& ctrl)
  {
    configureAcceleration(ctrl);

    if (ctrl.accelerationMode() == AccelerationMode::kNoAcceleration)
      k_.setTempo(ctrl.tempo());

    updateFramesLeft(ctrl);
  }

  void RegularGenerator::onSynchronize(BeatStreamController& ctrl,
                                       double beat_dev, double tempo_dev)
  {
    k_.synchronize(beat_dev, tempo_dev, kKinematicsSyncTime);
    updateFramesLeft(ctrl);
  }

  void RegularGenerator::onMeterChanged(BeatStreamController& ctrl, bool meter_enabled_changed)
  {
    const Meter& meter = ctrl.meter();

    // play the accent pattern from the beginning, when accentuation was enabled
    bool turnover = meter_enabled_changed && ctrl.isMeterEnabled();
    k_.setBeats(meter.beats(), turnover);

    // If accent_point_ == true (i.e. we are about to play an accent), we check
    // if there is a matching accent in the new meter and set the current accent
    // accordingly
    bool accent_match = (accent_ * meter.division()) % division_saved_ == 0;
    if (accent_point_ && accent_match)
    {
      accent_ = std::fmod(std::round(k_.position() * meter.division()),
                          meter.division() * meter.beats());
    }
    else
    {
      accent_ = std::trunc(k_.position() * meter.division());
      accent_point_ = false;
    }
    updateFramesLeft(ctrl);
    division_saved_ = ctrl.meter().division();
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
    k_.reset();
    k_.setBeats(ctrl.meter().beats());
    k_.setTempo(ctrl.tempo());

    if (ctrl.accelerationMode() == AccelerationMode::kContinuous)
      k_.accelerate(ctrl.acceleration(), ctrl.targetTempo());

    accent_ = 0;
    accent_point_ = true;
    division_saved_ = ctrl.meter().division();
    hold_pos_ = 0;
    updateFramesLeft(ctrl);
  }

  void RegularGenerator::leave(BeatStreamController& ctrl)
  { }

  void RegularGenerator::cycle(BeatStreamController& ctrl,
                               const void*& data, size_t& bytes)
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

    // update k_, frames_left_, ...
    step(ctrl, frames_chunk);
  }

  void RegularGenerator::updateStatus(BeatStreamController& ctrl, StreamStatus& status)
  {
    const Meter& meter = ctrl.meter();
    const AccentPattern& accents = meter.accents();

    status.position = k_.position();
    status.tempo = k_.tempo();
    status.acceleration = k_.acceleration();
    status.next_accent = (accent_ + 1) % accents.size();

    const double kMicrosecondsFramesRatio = (double) std::micro::den / ctrl.spec().rate;

    status.next_accent_delay
      = microseconds((microseconds::rep) (frames_left_ * kMicrosecondsFramesRatio));

    status.state = kRegularGenerator;
  }

  void RegularGenerator::updateFramesLeft(BeatStreamController& ctrl)
  {
    const Meter& meter = ctrl.meter();

    double accent_position = 0.0; // in accent units
    if (accent_point_)
      accent_position = std::round(k_.position() * meter.division());
    else
      accent_position = std::floor(k_.position() * meter.division());

    double next_accent_position = (accent_position + 1.0) / meter.division(); // in beat units
    double distance = next_accent_position - k_.position();

    physics::seconds_dbl arrival_time = k_.arrival(distance);
    frames_left_ = std::lround(ctrl.spec().rate * arrival_time.count());
  }

  void RegularGenerator::step(BeatStreamController& ctrl, size_t frames_chunk)
  {
    const Meter& meter = ctrl.meter();
    const AccentPattern& accents = meter.accents();

    k_.step(framesToUsecs(frames_chunk, ctrl.spec()));

    assert(frames_left_ >= frames_chunk);
    frames_left_ -= frames_chunk;

    if (frames_left_ == 0)
    {
      accent_ = (accent_ + 1) % accents.size();
      accent_point_ = true;

      handleStepwiseAcceleration(ctrl);

      updateFramesLeft(ctrl);
    }
    else
    {
      accent_point_ = false;
    }
  }

  void RegularGenerator::handleStepwiseAcceleration(BeatStreamController& ctrl)
  {
    if (ctrl.accelerationMode() != AccelerationMode::kStepwise)
      return;

    if (!accent_point_)
      return;

    const Meter& meter = ctrl.meter();

    if (accent_ % meter.division() == 0)
    {
      hold_pos_ += 1;
      if (hold_pos_ >= ctrl.hold())
      {
        accelerateStepwise(ctrl);
        hold_pos_ = 0;
      }
    }
  }

  void RegularGenerator::accelerateStepwise(BeatStreamController& ctrl)
  {
    double tempo = k_.tempo();
    double tempo_diff = ctrl.targetTempo() - tempo;

    if (std::abs(ctrl.step()) <= std::abs(tempo_diff))
    {
      double step = std::copysign(ctrl.step() , tempo_diff);
      k_.setTempo(tempo + step);
    }
    else
    {
      k_.setTempo(ctrl.targetTempo());
    }
  }

  // DrainGenerator
  //
  void DrainGenerator::cycle(BeatStreamController& ctrl,
                             const void*& data, size_t& bytes)
  {
    // not implemented yet
  }

}//namespace audio
