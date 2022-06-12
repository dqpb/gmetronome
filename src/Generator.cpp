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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Generator.h"
#include "Filter.h"
#include <cmath>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  namespace
  {
    constexpr microseconds kMaxChunkDuration {80ms};
    constexpr microseconds kAvgChunkDuration {50ms};

    //helper
    double tempoToFrameTime(double tempo, const StreamSpec& spec)
    {
      double const kMinutesFramesRatio = (1.0 / 60.0) / spec.rate;
      return kMinutesFramesRatio * tempo;
    }
    double tempoFromFrameTime(double tempo, const StreamSpec& spec)
    {
      double const kFramesMinutesRatio = spec.rate / (1.0 / 60.0);
      return kFramesMinutesRatio * tempo;
    }
    double accelToFrameTime(double accel, const StreamSpec& spec)
    {
      double const kMinutesFramesRatio = (1.0 / 60.0) / spec.rate;
      return kMinutesFramesRatio * kMinutesFramesRatio * accel;
    }
    double accelFromFrameTime(double accel, const StreamSpec& spec)
    {
      double const kFramesMinutesRatio = spec.rate / (1.0 / 60.0);
      return kFramesMinutesRatio * kFramesMinutesRatio * accel;
    }

  }//unnamed namespace

  Generator::Generator(const StreamSpec& spec)
    : spec_{spec},
      maxChunkFrames_{ (int) usecsToFrames(kMaxChunkDuration, spec_) },
      avgChunkFrames_{ (int) usecsToFrames(kAvgChunkDuration, spec_) },
      tempo_{ tempoToFrameTime(120.0, spec_) },
      target_tempo_{tempo_},
      accel_{0.0},
      accel_saved_{0.0},
      meter_{kMeter1},
      sound_zero_(spec_, 2 * kMaxChunkDuration),
      current_beat_{0},
      next_accent_{0},
      frames_total_{0},
      frames_done_{0}
  {
    sounds_.insert(kAccentWeak, SoundParameters{});
    sounds_.insert(kAccentMid, SoundParameters{});
    sounds_.insert(kAccentStrong, SoundParameters{});
    sounds_.prepare(spec_);
  }

  void Generator::prepare(const StreamSpec& spec)
  {
    assert(spec.rate > 0);

    if (spec == spec_)
      return;

    maxChunkFrames_ = usecsToFrames(kMaxChunkDuration, spec);
    avgChunkFrames_ = usecsToFrames(kAvgChunkDuration, spec);

    // recompute motion parameters
    tempo_        = tempoToFrameTime(tempoFromFrameTime(tempo_, spec_), spec);
    target_tempo_ = tempoToFrameTime(tempoFromFrameTime(target_tempo_, spec_), spec);
    accel_        = accelToFrameTime(accelFromFrameTime(accel_, spec_), spec);
    accel_saved_  = accelToFrameTime(accelFromFrameTime(accel_saved_, spec_), spec);

    sound_zero_.resize(spec, 2 * kMaxChunkDuration);

    sounds_.prepare(spec);
    sounds_.apply();

    spec_ = spec;
  }

  void Generator::setTempo(double tempo)
  {
    tempo_ = tempoToFrameTime(tempo, spec_);
    recalculateAccelSign();
    recalculateFramesTotal();
  }

  void Generator::setTargetTempo(double target_tempo)
  {
    target_tempo_ = tempoToFrameTime(target_tempo, spec_);
    recalculateAccelSign();
    recalculateFramesTotal();
  }

  void Generator::setAccel(double accel)
  {
    accel_ = accel_saved_ = accelToFrameTime(accel, spec_);
    recalculateAccelSign();
    recalculateFramesTotal();
  }

  void Generator::swapMeter(Meter& meter)
  {
    double s_subdiv = meter_.division();
    double t_subdiv = meter.division();

    std::swap(meter_, meter);

    auto fmodulo = [](double a, double b) -> int
                     {
                       int ai = std::lround(a);
                       int bi = std::lround(b);
                       return (ai % bi + bi) % bi;
                     };

    double accent_ratio = (double) t_subdiv / s_subdiv;
    double s_next_accent = next_accent_;
    double s_prev_accent = s_next_accent - 1.;
    double s_accent = s_prev_accent + accel_ / 2. * (frames_done_ * frames_done_) + frames_done_ * tempo_ * s_subdiv;
    double t_accent = accent_ratio * s_accent;
    double t_next_accent = std::ceil( t_accent );
    double t_prev_accent = t_next_accent - 1;

    // compute frames_done
    if (accel_ == 0) {
      frames_done_ = (t_accent - t_prev_accent) / ( tempo_ * t_subdiv );
      frames_total_ = framesPerPulse(tempo_, target_tempo_, accel_, t_subdiv);
    }
    else {
      // TODO
      /*
        double p = 2.0 * tempo / accel;
        double q = -2.0 * delta_s / accel;
        double p_half = p / 2.;
        double radicand = p_half * p_half - q;
      */
      // ...

      // tempo_ = ...

      // temp. solution (remove)
      frames_done_ = (t_accent - t_prev_accent) / ( tempo_ * t_subdiv );

      // temp solution. tempo should be the old tempo at the last pulse
      frames_total_ = framesPerPulse(tempo_, target_tempo_, accel_, t_subdiv);
    }

    next_accent_ = fmodulo(t_next_accent, meter_.accents().size());
    if (next_accent_ >= meter_.accents().size())
      next_accent_ = 0;
  }

  void Generator::setSound(Accent accent, const SoundParameters& params)
  { sounds_.update(accent, params); }

  const Generator::Statistics& Generator::getStatistics() const
  { return stats_; }

  void Generator::recalculateAccelSign() {
    if (target_tempo_ == tempo_)
      accel_ = 0;
    else
      accel_ = std::copysign(accel_saved_, (target_tempo_ < tempo_) ? -1. : 1.);
  }

  void Generator::recalculateFramesTotal() {
    frames_total_ = framesPerPulse(tempo_, target_tempo_, accel_, meter_.division());
  }

  void Generator::recalculateMotionParameters() {
    std::tie(tempo_, accel_) = motionAfterNFrames(tempo_, target_tempo_, accel_, frames_done_);
  }

  std::pair<double, double> Generator::motionAfterNFrames(double tempo,
                                                          double target_tempo,
                                                          double accel,
                                                          size_t n_frames)
  {
    double new_tempo;
    double new_accel;

    if (accel == 0) {
      new_tempo = tempo;
      new_accel = 0;
    }
    else {
      new_tempo = accel * n_frames + tempo;
      if ( (accel > 0 && new_tempo >= target_tempo) || (accel < 0 && new_tempo <= target_tempo) )
      {
        new_tempo = target_tempo;
        new_accel = 0;
      }
      else {
        new_accel = accel;
      }
    }
    return {new_tempo,new_accel};
  }

  size_t Generator::framesPerPulse(double tempo,
                                   double target_tempo,
                                   double accel,
                                   unsigned subdiv)
  {
    double frames_total;

    tempo = tempo * subdiv;
    accel = accel * subdiv;
    target_tempo = target_tempo * subdiv;

    auto framesAfterCompoundMotion = [&tempo,&target_tempo,&accel](double t) -> size_t
                                       {
                                         double s = accel / 2. * t * t + tempo * t;
                                         return t + ( 1. - s ) / target_tempo;
                                       };

    if (accel == 0) {
      frames_total = (size_t) ( 1. / tempo );
    }
    else {
      double p = 2.0 * tempo / accel;
      double q = -2.0 / accel;
      double p_half = p / 2.;
      double radicand = p_half * p_half - q;
      double t = ( target_tempo - tempo ) / accel;

      if (accel > 0) {
        frames_total =  ( -p_half + sqrt(radicand) );
        if (t<frames_total) {
          frames_total = framesAfterCompoundMotion(t);
        }
      }
      else {
        if (radicand < 0) {
          frames_total = framesAfterCompoundMotion(t);
        }
        else {
          frames_total = (size_t) ( -p_half - sqrt(radicand) );
          if (t<frames_total) {
            frames_total = framesAfterCompoundMotion(t);
          }
        }
      }
    }
    return frames_total;
  }

  void Generator::updateStatistics()
  {
    double tempo, accel;
    std::tie(tempo, accel) = motionAfterNFrames(tempo_, target_tempo_, accel_, frames_done_);

    stats_.current_tempo = tempoFromFrameTime(tempo, spec_);
    stats_.current_accel = accelFromFrameTime(accel, spec_);

    int current_accent = (next_accent_ + meter_.accents().size() - 1) % meter_.division();

    stats_.current_beat = current_beat_
      + (current_accent + (double) frames_done_ / frames_total_) / meter_.division();

    stats_.next_accent = next_accent_;

    int frames_left = std::max(0, frames_total_ - frames_done_);

    const double kMicrosecondsFramesRatio = 1000000.0 / spec_.rate;

    stats_.next_accent_delay
      = microseconds((microseconds::rep) (frames_left * kMicrosecondsFramesRatio));
  }

  void Generator::start(const void*& data, size_t& bytes)
  {
    sounds_.apply();

    current_beat_ = -1;
    next_accent_  = 0;

    frames_total_ = 2 * maxChunkFrames_;
    frames_done_  = 0;

    updateStatistics();

    data = sound_zero_.data();
    bytes = frames_total_ * frameSize(spec_);

    frames_total_ = 0;
    frames_done_ = 0;
  }

  void Generator::stop(const void*& data, size_t& bytes)
  {
    stats_.current_tempo = 0;
    stats_.current_accel = 0;
    stats_.current_beat = 0.0;
    stats_.next_accent = 0;
    stats_.next_accent_delay = 0us;

    data = sound_zero_.data();
    bytes = 0;
  }

  void Generator::cycle(const void*& data, size_t& bytes)
  {
    const AccentPattern& accents = meter_.accents();
    size_t frames_chunk = 0;

    int frames_left = frames_total_ - frames_done_;

    if (frames_left <= 0)
    {
      if (!accents.empty())
      {
        switch (accents[next_accent_])
        {
        case kAccentStrong:
        case kAccentMid:
        case kAccentWeak:
        {
          const auto& buffer = sounds_[accents[next_accent_]];
          frames_chunk = buffer.size() / frameSize(spec_);
          data = buffer.data();
          bytes = buffer.size();
        }
        break;

        default:
        {
          frames_chunk = avgChunkFrames_;
          data = sound_zero_.data();
          bytes = avgChunkFrames_ * frameSize(spec_);
        }
        break;
        };

        if (next_accent_ % meter_.division() == 0)
          ++current_beat_;

        if (++next_accent_ == accents.size())
          next_accent_ = 0;
      }
      recalculateMotionParameters();
      frames_done_ = 0;
      recalculateFramesTotal();
    }
    else if (frames_left <= maxChunkFrames_) {
      frames_chunk = frames_left;
      data = sound_zero_.data();
      bytes = frames_left * frameSize(spec_);
    }
    else {
      frames_chunk = frames_left / lround( (double) frames_left / avgChunkFrames_ );
      data = sound_zero_.data();
      bytes = frames_chunk * frameSize(spec_);
    }

    frames_done_ += frames_chunk;

    // std::cerr << "total: " << frames_total_
    //           << "\tdone: " << frames_done_
    //           << "\tchunk: " << frames_chunk
    //           << "\tnext: " << next_accent_
    //           << std::endl;

    updateStatistics();
  }

}//namespace audio
