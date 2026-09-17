/*
 * Copyright (C) 2021-2026 The GMetronome Team
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

    size_t preferredChunkSize(size_t frames, size_t max_chunk_size, size_t avg_chunk_size)
    {
      if (frames <= max_chunk_size)
        return frames;
      else {
        auto n_chunks = frames / avg_chunk_size + (frames % avg_chunk_size != 0);
        return  frames / n_chunks;
      }
    }

    enum class PosHint {
      kEnd,
      kBegin,
      kMiddle
    };

    /**
     * Compute the accent index from the current oscillator position.
     */
    size_t currentAccent(const physics::BeatKinematics& k, const Meter& m, PosHint hint)
    {
      double pos = k.position() * m.division(); // position in accent units

      if (hint == PosHint::kMiddle)
        return static_cast<size_t>(pos); // trunc

      // handle boundary cases
      const size_t pos_near = static_cast<size_t>(pos + 0.5); // round
      const size_t n = static_cast<size_t>(m.beats() * m.division());

      if (hint == PosHint::kEnd)
        return (pos_near == 0) ? (n - 1) : (pos_near - 1);

      return (pos_near >= n) ? 0 : pos_near;
    }

    /**
     * Compute the distance from the current oscillator position to the
     * next accent (in beat units).
     */
    double distanceToNextAccent(const physics::BeatKinematics& k, const Meter& m, PosHint hint)
    {
      double pos = k.position() * m.division(); // position in accent units
      double dist = 0.0;

      if (hint == PosHint::kMiddle) {
        dist = 1.0 - (pos - std::floor(pos));
      }
      else {
        const double pos_near = std::floor(pos + 0.5);
        dist = (hint == PosHint::kEnd) ?
          std::max(0.0, (pos_near - pos))
          : ((pos_near + 1.0) - pos);
      }
      return dist / m.division(); // in beat units
    }

    /**
     * Compute the number of frames to reach the next accent from
     * the current oscillator position.
     */
    size_t framesTillNextAccent(const physics::BeatKinematics& k,
                                const Meter& m,
                                audio::SampleRate rate,
                                PosHint hint)
    {
      double distance = distanceToNextAccent(k, m, hint);
      physics::seconds_dbl arrival_time = k.arrival(distance);
      return static_cast<size_t>(rate * arrival_time.count() + 0.5); // round
    }
  } // unnamed namespace

  //
  // FillBufferGenerator
  //
  void FillBufferGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_size_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                               ctrl.sound(kAccentOff).buffer.frames());

    avg_chunk_size_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());

    double percentage = (frames_total_ > 0) ? 100.0 * frames_done_ / frames_total_ : 0;

    frames_total_ = usecsToFrames(kFillBufferDuration, ctrl.spec());
    frames_done_ = frames_total_ / 100.0 * percentage;
  }

  void FillBufferGenerator::enter(BeatStreamController& ctrl)
  {
    frames_done_ = 0;
    updateStatus(ctrl);
  }

  void FillBufferGenerator::leave(BeatStreamController& ctrl)
  {
    // nothing
  }

  void FillBufferGenerator::cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes)
  {
    size_t frames_left = frames_total_ - frames_done_;
    size_t chunk_size = preferredChunkSize(frames_left, max_chunk_size_, avg_chunk_size_);

    data = ctrl.sound(kAccentOff).buffer.data();
    bytes = chunk_size * frameSize(ctrl.spec());

    frames_done_ += chunk_size;

    updateStatus(ctrl);

    if (frames_done_ >= frames_total_)
      switchGenerator(ctrl, kPreCountGenerator);
  }

  void FillBufferGenerator::updateStatus(BeatStreamController& ctrl)
  {
    auto& status = ctrl.status();

    size_t frames_left = frames_total_ - frames_done_;

    physics::seconds_dbl time_left {static_cast<double>(frames_left) / ctrl.spec().rate};

    status.position = -ctrl.countIn() - ctrl.tempo() * time_left.count() / 60.0;
    status.tempo = ctrl.tempo();
    status.acceleration = 0.0;
    status.hold = 0;
    status.accent = -1;
    status.next_accent_delay = std::chrono::duration_cast<microseconds>(time_left);
    status.peak = {0.0f, 0.0f};
    status.clip = {0, 0};
    status.generator = kFillBufferGenerator;
  }

  // PreCountGenerator
  //
  void PreCountGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_size_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                               ctrl.sound(kAccentOff).buffer.frames());

    avg_chunk_size_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());
  }

  void PreCountGenerator::enter(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    k.reset();

    ctrl.peakBallistics().reset();
    ctrl.clipCounter().reset();

    if (ctrl.countIn() > 0)
    {
      k.setTempo(ctrl.tempo());
      k.setBeats(100);
      frames_left_ = 0;
    }
    else
    {
      switchGenerator(ctrl, kRegularGenerator); // skip this generator
    }
  }

  void PreCountGenerator::leave(BeatStreamController& ctrl)
  {
    // nothing
  }

  void PreCountGenerator::onTempoChanged(BeatStreamController& ctrl, TempoMode old_mode)
  {
    auto& k = ctrl.kinematics();
    const auto& m = kMeter1;
    const auto rate = ctrl.spec().rate;

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

    if (frames_left_ != 0)
      frames_left_ = framesTillNextAccent(k, m, rate, PosHint::kMiddle);
  }

  void PreCountGenerator::onCountInChanged(BeatStreamController& ctrl)
  {
    // nothing
  }

  void PreCountGenerator::cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes)
  {
    size_t pref_chunk_size = 0;
    const Sound& sound = (frames_left_ == 0) ? ctrl.sound(kAccentMid) : ctrl.sound(kAccentOff);
    const auto& sound_buffer = sound.buffer;

    if (frames_left_ == 0) // play sound
    {
      pref_chunk_size = sound_buffer.frames();
      data = sound_buffer.data();
    }
    else // play silence
    {
      pref_chunk_size = preferredChunkSize(frames_left_, max_chunk_size_, avg_chunk_size_);
      assert(pref_chunk_size <= sound_buffer.frames());
      data = sound_buffer.data();
    }

    // Update frames_left_ and kinematics and get the corrected chunk size.
    size_t chunk_size = step(ctrl, pref_chunk_size);
    bytes = chunk_size * frameSize(ctrl.spec());

    // Update peak ballistics and clip counter.
    auto delta_time = framesToUsecs(chunk_size, ctrl.spec());
    ctrl.peakBallistics().update(sound.peak, delta_time);
    ctrl.clipCounter().update(sound.clipped);

    // Update status.
    updateStatus(ctrl);

    // Switch generator when we are done.
    if (frames_left_ == 0 && std::round(ctrl.kinematics().position()) >= ctrl.countIn())
      switchGenerator(ctrl, kRegularGenerator);
  }

  size_t PreCountGenerator::step(BeatStreamController& ctrl, size_t pref_chunk_size)
  {
    size_t chunk_size = pref_chunk_size;

    if (frames_left_ == 0)
    {
      auto& k = ctrl.kinematics();
      const auto& m = kMeter1;
      const auto rate = ctrl.spec().rate;

      frames_left_ = framesTillNextAccent(k, m, rate, PosHint::kBegin);

      if (frames_left_ < pref_chunk_size)
        chunk_size = frames_left_;
    }

    auto delta_time = framesToUsecs(chunk_size, ctrl.spec());
    ctrl.kinematics().step(delta_time);

    assert(chunk_size <= frames_left_);
    frames_left_ -= chunk_size;

    return chunk_size;
  }

  void PreCountGenerator::updateStatus(BeatStreamController& ctrl)
  {
    auto& status = ctrl.status();
    auto& k = ctrl.kinematics();

    if (k.position() <= ctrl.countIn())
    {
      status.position = k.position() - ctrl.countIn();
    }
    else     // if position already exceeded the requested count-in
    {        // (i.e. the count-in was reduced when playing the last beat)
      double p_int;
      double p_frac = std::modf(k.position(), &p_int);
      status.position = p_frac - 1.0;
    }
    status.tempo        = k.tempo();
    status.mode         = k.isSynchronizing() ? TempoMode::kSync : TempoMode::kConstant;
    status.acceleration = k.acceleration();
    status.hold         = 0;

    if (frames_left_ == 0)
      // Kinematics has high module (no wrap around), so this is safe.
      status.accent = std::max(0.0, std::round(k.position()) - 1.0);
    else
      status.accent = std::floor(k.position());

    const double kMicrosecondsFramesRatio =
      static_cast<double>(std::micro::den) / ctrl.spec().rate;

    status.next_accent_delay
      = microseconds((microseconds::rep) (frames_left_ * kMicrosecondsFramesRatio));

    status.peak = ctrl.peakBallistics().peak();
    status.clip = ctrl.clipCounter().count();

    status.generator = kPreCountGenerator;
  }

  //
  // RegularGenerator
  //
  void RegularGenerator::prepare(BeatStreamController& ctrl)
  {
    max_chunk_size_ = std::min(usecsToFrames(kMaxChunkDuration, ctrl.spec()),
                               ctrl.sound(kAccentOff).buffer.frames());

    avg_chunk_size_ = usecsToFrames(kAvgChunkDuration, ctrl.spec());
  }

  void RegularGenerator::enter(BeatStreamController& ctrl)
  {
    auto& k = ctrl.kinematics();

    k.setBeats( ctrl.meter().beats(), physics::BeatKinematics::PositionMode::kZero );

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

    hold_ = 0;
    frames_left_ = 0;
  }

  void RegularGenerator::leave(BeatStreamController& ctrl)
  {
    // nothing
  }

  void RegularGenerator::onTempoChanged(BeatStreamController& ctrl, TempoMode old_mode)
  {
    auto& k = ctrl.kinematics();
    const Meter& m = ctrl.meter();
    const auto rate = ctrl.spec().rate;

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

      recomputeStepwise(ctrl);
      break;

    case TempoMode::kSync:
      k.synchronize(ctrl.syncBeats(), ctrl.syncTempo(), ctrl.syncTime());
      break;
    };

    if (frames_left_ != 0)
      frames_left_ = framesTillNextAccent(k, m, rate, PosHint::kMiddle);
  }

  void RegularGenerator::onMeterChanged(BeatStreamController& ctrl,
                                        const Meter& old_m, bool enabled_changed)
  {
    auto& k = ctrl.kinematics();
    const Meter& new_m = ctrl.meter();
    const auto rate = ctrl.spec().rate;

    bool accent_match = false;

    if (frames_left_ == 0) {
      size_t old_accent =
        currentAccent(k, old_m, (frames_left_ == 0) ? PosHint::kBegin : PosHint::kMiddle);

      accent_match = (old_accent * new_m.division()) % old_m.division() == 0;
    }

    if (k.beats() != new_m.beats())
    {
      // Update kinematics: if accentuation has just been enabled or the kinematic's
      // current position already exceeded the new number of beats, we also reset the
      // position to play the first accent next.
      if ((enabled_changed && ctrl.isMeterEnabled()) || (k.position() >= new_m.beats()))
        k.setBeats(new_m.beats(), physics::BeatKinematics::PositionMode::kRollover);
      else
        k.setBeats(new_m.beats());
    }

    // Update hold_.
    if (ctrl.mode() == TempoMode::kStepwise)
      recomputeStepwise(ctrl);

    // Update frames_left_: we skip this, if we are about to play an accent
    // (i.e. frames_left_ is zero) and there is a matching accent in the new meter
    // (see computation of accent_match above).
    if (!accent_match)
      frames_left_ = framesTillNextAccent(k, new_m, rate, PosHint::kMiddle);
  }

  void RegularGenerator::cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes)
  {
    size_t pref_chunk_size = 0;
    const Sound* sound = nullptr;

    if (frames_left_ == 0) // play sound
    {
      const auto& k = ctrl.kinematics();
      const Meter& m = ctrl.meter();
      const AccentPattern& accent_pattern = m.accents();

      size_t accent_index = currentAccent(k, m, PosHint::kBegin);

      assert(accent_index < accent_pattern.size());
      sound = &ctrl.sound(accent_pattern[accent_index]);

      pref_chunk_size = sound->buffer.frames();
      data = sound->buffer.data();
    }
    else // play silence
    {
      sound = &ctrl.sound(kAccentOff);
      pref_chunk_size = preferredChunkSize(frames_left_, max_chunk_size_, avg_chunk_size_);
      data = sound->buffer.data();
    }
    // Update frames_left_ and kinematics and get the corrected chunk size.
    size_t chunk_size = step(ctrl, pref_chunk_size);
    bytes = chunk_size * frameSize(ctrl.spec());

    // Update peak ballistics and clip counter.
    auto delta_time = framesToUsecs(chunk_size, ctrl.spec());
    ctrl.peakBallistics().update(sound->peak, delta_time);
    ctrl.clipCounter().update(sound->clipped);

    // Update status.
    updateStatus(ctrl);
  }

  size_t RegularGenerator::step(BeatStreamController& ctrl, size_t pref_chunk_size)
  {
    const auto& k = ctrl.kinematics();
    const Meter& m = ctrl.meter();
    const auto rate = ctrl.spec().rate;
    size_t chunk_size = pref_chunk_size;

    if (frames_left_ == 0)
    {
      // Handle stepwise acceleration on every beat crossing.
      size_t accent = currentAccent(k, m, PosHint::kBegin);
      if (accent % m.division() == 0)
        stepStepwise(ctrl);

      frames_left_ = framesTillNextAccent(k, m, rate, PosHint::kBegin);

      if (frames_left_ < pref_chunk_size)
        chunk_size = frames_left_;
    }

    auto delta_time = framesToUsecs(chunk_size, ctrl.spec());
    ctrl.kinematics().step(delta_time);

    assert(chunk_size <= frames_left_);
    frames_left_ -= chunk_size;

    return chunk_size;
  }

  void RegularGenerator::updateStatus(BeatStreamController& ctrl)
  {
    auto& status = ctrl.status();
    const auto& k = ctrl.kinematics();

    status.position     = k.position();
    status.tempo        = k.tempo();
    status.mode         = effectiveMode(ctrl);
    status.acceleration = k.acceleration();
    status.hold         = hold_;

    if (frames_left_ == 0)
      status.accent = currentAccent(k, ctrl.meter(), PosHint::kEnd);
    else
      status.accent = currentAccent(k, ctrl.meter(), PosHint::kMiddle);

    const double kMicrosecondsFramesRatio = (double) std::micro::den / ctrl.spec().rate;

    status.next_accent_delay
      = microseconds((microseconds::rep) (frames_left_ * kMicrosecondsFramesRatio));

    status.peak = ctrl.peakBallistics().peak();
    status.clip = ctrl.clipCounter().count();

    status.generator = kRegularGenerator;
  }

  void RegularGenerator::stepStepwise(BeatStreamController& ctrl)
  {
    if (hold_ == 0)
      hold_ = ctrl.hold(); // initialize
    else if (--hold_; hold_ == 0) {
      if (ctrl.mode() == TempoMode::kStepwise)
        accelerateStepwise(ctrl);

      hold_ = ctrl.hold();
    }
  }

  void RegularGenerator::recomputeStepwise(const BeatStreamController& ctrl)
  {
    const auto& k = ctrl.kinematics();
    const auto& m = ctrl.meter();

    if (int gcd = std::gcd(ctrl.hold(), m.beats()); gcd > 1)
    {
      int accent = (frames_left_ == 0) ?
        currentAccent(k, m, PosHint::kEnd) : currentAccent(k, m, PosHint::kMiddle);

      int beat = accent / m.division();                   // current beat
      int rem_beat = (m.beats() - beat);                  // remaining beats in measure
      int offset = std::remainder(rem_beat - static_cast<int>(hold_), gcd); // displacement (mod gcd)

      hold_ = aux::math::modulo(static_cast<int>(hold_) + offset,
                                static_cast<int>(ctrl.hold()));
      if (hold_ == 0)
        hold_ = ctrl.hold();
    }
    else
      hold_ = std::min(hold_, ctrl.hold());
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
      k.setTempo(ctrl.target());
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

  //
  // DrainGenerator
  //
  void DrainBufferGenerator::cycle(BeatStreamController& ctrl,
                                   const void*& data, size_t& bytes)
  {
    // not implemented yet
  }

}//namespace audio
