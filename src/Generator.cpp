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

#include "Generator.h"

#include <cmath>
#include <cassert>
#include <iostream>

namespace audio {

  Generator::Generator(SampleSpec spec,
                       microseconds maxChunkDuration,
                       microseconds avgChunkDuration)
    : kSampleSpec_(spec),
      kMaxChunkDuration_(maxChunkDuration),
      kAvgChunkDuration_(avgChunkDuration),
      kMaxChunkFrames_(usecsToFrames(kMaxChunkDuration_, kSampleSpec_)),
      kAvgChunkFrames_(usecsToFrames(kAvgChunkDuration_, kSampleSpec_)),
      kMinutesFramesRatio_(1. / (60. * kSampleSpec_.rate)),
      kMicrosecondsFramesRatio_(1000000. / kSampleSpec_.rate),
      kFramesMinutesRatio_(1. / kMinutesFramesRatio_),
      kFramesMicrosecondsRatio_(1. / kMicrosecondsFramesRatio_),
      tempo_(convertTempoToFrameTime(120.)),
      target_tempo_(convertTempoToFrameTime(120.)),
      accel_(convertAccelToFrameTime(0.0)),
      meter_({kSingleMeter, kNoDivision, {kAccentMid}}),
      sound_zero_(2 * kMaxChunkDuration_, kSampleSpec_),
      sound_strong_(sound_zero_),
      sound_mid_(sound_zero_),
      sound_weak_(sound_zero_),
      next_accent_(0),
      frames_total_(0),
      frames_done_(0),
      stats_({0,0,0,0us})
  {}

  Generator::~Generator()
  {}

  void Generator::setTempo(double tempo)
  {
    tempo_ = convertTempoToFrameTime(tempo);
    recalculateAccelSign();
    recalculateFramesTotal();
  }
  
  void Generator::setTargetTempo(double target_tempo)
  {
    target_tempo_ = convertTempoToFrameTime(target_tempo);
    recalculateAccelSign();
    recalculateFramesTotal();
  }
  
  void Generator::setAccel(double accel)
  {
    accel_ = convertAccelToFrameTime(accel);
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
  
  void Generator::swapSoundStrong(Buffer& sound)
  {
    std::swap(sound_strong_,sound);
  }
  
  void Generator::swapSoundMid(Buffer& sound)
  {
    std::swap(sound_mid_,sound);
  }

  void Generator::swapSoundWeak(Buffer& sound)
  {
    std::swap(sound_weak_,sound);
  }
  
  const Generator::Statistics& Generator::getStatistics() const
  {
    return stats_;
  }
      
  double Generator::convertTempoToFrameTime(double tempo) {
    return kMinutesFramesRatio_ * tempo;
  }

  double Generator::convertAccelToFrameTime(double accel) {
    return kMinutesFramesRatio_ * kMinutesFramesRatio_ * accel;
  }

  void Generator::recalculateAccelSign() {
    if (target_tempo_ == tempo_)
      accel_ = 0.;
    else 
      accel_ = std::copysign(accel_, (target_tempo_ < tempo_) ? -1. : 1.);
  }

  void Generator::recalculateFramesTotal() {
    frames_total_ = framesPerPulse(tempo_, target_tempo_, accel_, meter_.division());
  }

  void Generator::recalculateTempo() {
    tempo_ = tempoAfterNFrames(tempo_, target_tempo_, accel_, frames_done_);
  }

  double Generator::tempoAfterNFrames(double tempo,
                                      double target_tempo,
                                      double accel,
                                      size_t n_frames)
  {
    double new_tempo;
    if (accel == 0) {
      new_tempo = tempo;
    }
    else {
      new_tempo = accel * n_frames + tempo;
      if ( (accel > 0 && new_tempo > target_tempo) || (accel < 0 && new_tempo < target_tempo) )
        new_tempo = target_tempo;
    }
    return new_tempo;
  }

  double Generator::accelAfterNFrames(double tempo,
                                      double target_tempo,
                                      double accel,
                                      size_t n_frames)
  {
    double new_accel;
    if (accel == 0) {
      new_accel = 0;
    }
    else {
      double new_tempo = accel * n_frames + tempo;
      if ( (accel > 0 && new_tempo >= target_tempo) || (accel < 0 && new_tempo <= target_tempo) )
        new_accel = 0;
      else
        new_accel = accel;
    }
    return new_accel;
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
    stats_.current_tempo = 
      tempoAfterNFrames(tempo_, target_tempo_, accel_, frames_done_) * kFramesMinutesRatio_;
    
    stats_.current_accel =
      accelAfterNFrames(tempo_, target_tempo_, accel_, frames_done_) * kFramesMinutesRatio_;
    
    stats_.next_accent = next_accent_;

    int frames_left = std::max(0, frames_total_ - frames_done_);
    
    stats_.next_accent_time
      = microseconds((microseconds::rep) (frames_left * kMicrosecondsFramesRatio_));
  }
  
  void Generator::start(const void*& data, size_t& bytes)
  {
    next_accent_  = 0;
    
    frames_total_ = 2 * kMaxChunkFrames_;
    frames_done_  = 0;

    updateStatistics();
    
    data = &(sound_zero_[0]);
    bytes = frames_total_ * frameSize(kSampleSpec_);
    
    frames_total_ = 0;
    frames_done_ = 0;
  }

  void Generator::stop(const void*& data, size_t& bytes)
  {
    stats_.current_tempo = 0;
    stats_.current_accel = 0;
    stats_.next_accent = 0;
    stats_.next_accent_time = 0us;

    data = &(sound_zero_[0]);
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
          frames_chunk = sound_strong_.size() / frameSize(kSampleSpec_); 
          data = &(sound_strong_[0]);
          bytes = sound_strong_.size();
          break;

        case kAccentMid:
          frames_chunk = sound_mid_.size() / frameSize(kSampleSpec_); 
          data = &(sound_mid_[0]);
          bytes = sound_mid_.size();
          break;

        case kAccentWeak:
          frames_chunk = sound_weak_.size() / frameSize(kSampleSpec_); 
          data = &(sound_weak_[0]);
          bytes = sound_weak_.size();
          break;

        default:
          frames_chunk = kAvgChunkFrames_;
          data = &(sound_zero_[0]);
          bytes = kAvgChunkFrames_ * frameSize(kSampleSpec_);
          break;
        };
        if (++next_accent_ == accents.size())
          next_accent_ = 0;
      }
      recalculateTempo();
      frames_done_ = frames_done_ - frames_total_;
      recalculateFramesTotal();
    }
    else if (frames_left <= kMaxChunkFrames_) {
      frames_chunk = frames_left;
      data = &(sound_zero_[0]);
      bytes = frames_left * frameSize(kSampleSpec_);
    }
    else {
      frames_chunk = frames_left / lround( (double) frames_left / kAvgChunkFrames_ );
      data = &(sound_zero_[0]);
      bytes = frames_chunk * frameSize(kSampleSpec_);
    }

    frames_done_ += frames_chunk;

    updateStatistics();
  }

  Buffer generateSound(double frequency,
                       double volume,
                       SampleSpec spec,
                       microseconds duration)
  {
    assert(spec.channels == 1);
    assert(spec.format == SampleFormat::S16LE);
    
    volume = std::min( std::max(volume, 0.), 1.0);
    
    using namespace std::chrono_literals;
    
    Buffer buffer(duration, spec);

    if (volume > 0)
    {
      double sine_fac = 2 * M_PI * frequency / spec.rate;
      auto n_frames = buffer.frames();
      double one_over_n_frames = 1. / n_frames;
      double volume_drop_exp = 2. / volume;
      auto frameSize = audio::frameSize(spec);
      
      for(size_t frame = 0, buffer_index = 0;
          frame < n_frames;
          ++frame, buffer_index+=frameSize)
      {
        double v = volume * std::pow( 1. - one_over_n_frames * frame, volume_drop_exp);
        double amplitude = v * std::sin(sine_fac * frame);
        int16_t pcm =  amplitude * std::numeric_limits<int16_t>::max();
	
        buffer[buffer_index] = pcm & 0xff;
        buffer[buffer_index+1] = (pcm>>8) & 0xff;	
      }
    }
    return buffer;
  }

}//namespace
