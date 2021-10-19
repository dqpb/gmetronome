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
#include <glib.h>

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
      sound_zero_(kMaxChunkDuration_, kSampleSpec_),
      sound_low_(sound_zero_),
      sound_mid_(sound_zero_),
      sound_high_(sound_zero_),
      in_tempo_(tempo_),
      in_target_tempo_(target_tempo_),
      in_accel_(accel_),
      in_meter_(meter_),
      in_sound_low_(sound_zero_),
      in_sound_mid_(sound_zero_),
      in_sound_high_(sound_zero_),
      out_current_tempo_(0.0),
      out_current_accel_(0.0),
      out_next_accent_(0),
      out_latency_(0),
      condition_var_flag_(false),
      next_accent_(0),
      frames_done_(0),
      frames_left_(0),
      frames_chunk_(0),
      frames_total_(0)
  {
    tempo_import_flag_.test_and_set();
    accel_import_flag_.test_and_set();
    target_tempo_import_flag_.test_and_set();
    meter_import_flag_.test_and_set();
    sound_low_import_flag_.test_and_set();
    sound_mid_import_flag_.test_and_set();
    sound_high_import_flag_.test_and_set();
    audio_sink_import_flag_.test_and_set();
  }

  Generator::~Generator()
  {}

  void Generator::setTempo(double tempo)
  {
    in_tempo_.store(tempo);
    tempo_import_flag_.clear(std::memory_order_release);
  }
  
  void Generator::setTargetTempo(double target_tempo)
  {
    in_target_tempo_.store(target_tempo);
    target_tempo_import_flag_.clear(std::memory_order_release);
  }
  
  void Generator::setAccel(double accel)
  {
    in_accel_.store(accel);
    accel_import_flag_.clear(std::memory_order_release);
  }

  void Generator::setMeter(const Meter& meter)
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      in_meter_ = meter;
    }
    meter_import_flag_.clear(std::memory_order_release);
  }
  
  void Generator::setMeter(Meter&& meter)
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      in_meter_ = std::move(meter);
    }
    meter_import_flag_.clear(std::memory_order_release);
  }

  void Generator::setSoundHigh(Buffer&& sound)
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      std::swap(in_sound_high_,sound);
    }
    sound_high_import_flag_.clear(std::memory_order_release);
  }
  
  void Generator::setSoundMid(Buffer&& sound)
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      std::swap(in_sound_mid_,sound);
    }
    sound_mid_import_flag_.clear(std::memory_order_release);
  }

  void Generator::setSoundLow(Buffer&& sound)
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      std::swap(in_sound_low_,sound);
    }
    sound_low_import_flag_.clear(std::memory_order_release);
  }
  
  Statistics Generator::getStatistics() const
  {
    Statistics stats;
    
    stats.current_tempo = out_current_tempo_.load();
    stats.current_accel = out_current_accel_.load();
    
    uint64_t tmp = out_next_accent_.load();
    
    stats.next_accent = ((tmp >> 56) & 0x00000000000000ff);
    stats.next_accent_time = (tmp & 0x00ffffffffffffff);

    stats.latency = out_latency_.load();
    
    return stats;
  }

  std::unique_ptr<AbstractAudioSink>
  Generator::swapSink(std::unique_ptr<AbstractAudioSink> sink)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    std::swap(in_audio_sink_,sink);

    audio_sink_import_flag_.clear(std::memory_order_release);

    bool cv_status = condition_var_.wait_for( lock,
                                              2 * kMaxChunkDuration_,
                                              [&] {return condition_var_flag_;} );
    condition_var_flag_ = false;
    
    if ( !cv_status )
    {
      std::swap(audio_sink_, in_audio_sink_);
      audio_sink_import_flag_.test_and_set(std::memory_order_acquire);
    }
    return std::move(in_audio_sink_);
  }
  
  void Generator::importTempo()
  {
    tempo_ = convertTempoToFrameTime(in_tempo_.load());
    accel_ = convertAccelToFrameTime(in_accel_.load());

    recalculateAccelSign();
    recalculateFramesTotal();
  }
  
  void Generator::importTargetTempo()
  {
    target_tempo_ = convertTempoToFrameTime(in_target_tempo_.load());
    accel_ = convertAccelToFrameTime(in_accel_.load());

    recalculateAccelSign();
    recalculateFramesTotal();
  }
  
  void Generator::importAccel()
  {
    accel_ = convertAccelToFrameTime(in_accel_.load());

    recalculateAccelSign();
    recalculateFramesTotal();
  }

  void Generator::importMeter()
  {
    std::lock_guard<std::mutex> guard(mutex_);

    double s_subdiv = meter_.division();
    double t_subdiv = in_meter_.division();

    std::swap(meter_, in_meter_);
    
    // TODO: recalculate the current index to gain a smooth transition between the 
    //       old and the new pattern, ie. keep beats consistent.
    //
    // This seems to be a little harder to make in a clean way but it is entirely
    // possible with the known information.

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
      /*
        double p = 2.0 * tempo / accel;
        double q = -2.0 * delta_s / accel;
        double p_half = p / 2.;
        double radicand = p_half * p_half - q;
      */
      // ...

      // tempo = ...
	    
      // temp. solution (remove) 
      frames_done_ = (t_accent - t_prev_accent) / ( tempo_ * t_subdiv );

      // temp solution. tempo should be the old tempo at the last pulse
      frames_total_ = framesPerPulse(tempo_, target_tempo_, accel_, t_subdiv);
    }

    next_accent_ = fmodulo(t_next_accent, meter_.accents().size());
    if (next_accent_ >= meter_.accents().size())
      next_accent_ = 0;
    
    frames_left_ = frames_total_ - frames_done_;

    // TODO: there is a race condition regarding out_next_accent_ which leads to
    // a potential data inconsistency after a pattern change followed by a reqest
    // of the next accent (the returned index could be out of pattern range). 
    // A solution might introduce another condition variable in case of a pending
    // pattern change during a next accent request.
    //
    // out_next_accent_.store(
    //   ( (next_accent_ << 16) & 0xffff0000 ) | ( frames_left_ & 0x0000ffff ) );
  }
  
  void Generator::importSoundHigh()
  {
    std::lock_guard<std::mutex> guard(mutex_);
    std::swap(sound_high_, in_sound_high_);
  }
  
  void Generator::importSoundMid()
  {
    std::lock_guard<std::mutex> guard(mutex_);
    std::swap(sound_mid_, in_sound_mid_);
  }
  
  void Generator::importSoundLow()
  {
    std::lock_guard<std::mutex> guard(mutex_);
    std::swap(sound_low_, in_sound_low_);
  }

  void Generator::importAudioSink()
  {
    {
      std::lock_guard<std::mutex> guard(mutex_);
      std::swap(audio_sink_, in_audio_sink_);
      condition_var_flag_ = true;
    }
    condition_var_.notify_all();
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

  void Generator::recalculateLatency() {
    latency_ = audio_sink_->latency();
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

  void Generator::exportCurrentTempo()
  {
    out_current_tempo_.store(
      tempoAfterNFrames(tempo_, target_tempo_, accel_, frames_done_) * kFramesMinutesRatio_ );      
  }

  void Generator::exportCurrentAccel()
  {
    out_current_accel_.store(
      accelAfterNFrames(tempo_, target_tempo_, accel_, frames_done_) * kFramesMinutesRatio_ );      
  }

  void Generator::exportNextAccent()
  {
    uint64_t now = g_get_monotonic_time();
    uint64_t time = now + latency_ + (frames_left_ * kMicrosecondsFramesRatio_);

    uint64_t accent = next_accent_;
    out_next_accent_.store(
      ( (accent << 56) & 0xff00000000000000 ) | ( time & 0x00ffffffffffffff ) );
  }

  void Generator::exportLatency()
  {
    out_latency_.store(latency_);
  }
  
  void Generator::start()
  {
    next_accent_ = 0;

    out_current_tempo_ = 0.0;
    out_current_accel_ = 0.0;
    out_next_accent_ = 0;
    out_latency_ = 0;
    
    if (!tempo_import_flag_.test_and_set(std::memory_order_acquire))
      tempo_ = convertTempoToFrameTime(in_tempo_.load());
    
    if (!target_tempo_import_flag_.test_and_set(std::memory_order_acquire))
      target_tempo_ = convertTempoToFrameTime(in_target_tempo_.load());

    // always reset acceleration
    accel_import_flag_.test_and_set(std::memory_order_acquire);
    accel_ = convertAccelToFrameTime(in_accel_.load());
    
    if (!meter_import_flag_.test_and_set(std::memory_order_acquire))
      std::swap(meter_, in_meter_);

    recalculateAccelSign();
        
    if (!sound_high_import_flag_.test_and_set(std::memory_order_acquire))
      std::swap(sound_high_, in_sound_high_);
      
    if (!sound_mid_import_flag_.test_and_set(std::memory_order_acquire))
      std::swap(sound_mid_, in_sound_mid_);
      
    if (!sound_low_import_flag_.test_and_set(std::memory_order_acquire))
      std::swap(sound_low_, in_sound_low_);    

    // we start by filling the audio buffer with zeros
    audio_sink_->write(&(sound_zero_[0]), kMaxChunkFrames_ * frameSize(kSampleSpec_));      
    
    frames_total_ = kMaxChunkFrames_;
    frames_done_ = 0;
    frames_left_ = frames_total_;
    frames_chunk_ = 0;

    // measure the latency and export the next accent info for the first accent
    recalculateLatency();
    exportLatency();
    exportNextAccent();
    
    // write the remaining frames 
    audio_sink_->write(&(sound_zero_[0]), frames_left_ * frameSize(kSampleSpec_));      
    
    recalculateFramesTotal();

    frames_done_ = 0;
    frames_left_ = 0;
    frames_chunk_ = 0;

    exportCurrentTempo();
    exportCurrentAccel();
  }

  void Generator::stop()
  {
    out_current_tempo_ = 0.0;
    out_current_accel_ = 0.0;
    out_next_accent_ = 0;
    out_latency_ = 0;
  }
  
  void Generator::cycle()
  {
    const AccentPattern& accents = meter_.accents();
    
    if (frames_left_ <= 0) {
      frames_chunk_ = 0;
      if (!accents.empty())
      {
        switch (accents[next_accent_])
        {
        case kAccentHigh:
          frames_chunk_ = sound_high_.size() / frameSize(kSampleSpec_); 
          audio_sink_->write(&(sound_high_[0]), sound_high_.size());
          break;
        case kAccentMid:
          frames_chunk_ = sound_mid_.size() / frameSize(kSampleSpec_); 
          audio_sink_->write(&(sound_mid_[0]), sound_mid_.size());
          break;
        case kAccentLow:
          frames_chunk_ = sound_low_.size() / frameSize(kSampleSpec_); 
          audio_sink_->write(&(sound_low_[0]), sound_low_.size());
          break;
        default:
          break;
        };
        if (++next_accent_ == accents.size())
          next_accent_ = 0;
      }
      frames_done_ = frames_chunk_;
    }
    else if (frames_left_ <= kMaxChunkFrames_) {
      frames_chunk_ = frames_left_;
      audio_sink_->write(&(sound_zero_[0]), frames_left_ * frameSize(kSampleSpec_));
      frames_done_ += frames_chunk_;
    }
    else {
      frames_chunk_ = frames_left_ / lround( (double) frames_left_ / kAvgChunkFrames_ );
      audio_sink_->write(&(sound_zero_[0]), frames_chunk_ * frameSize(kSampleSpec_));
      frames_done_ += frames_chunk_;
    }
    
    // apply at most one parameter import per cycle
    if (!tempo_import_flag_.test_and_set(std::memory_order_acquire))
      importTempo();
    else if (!target_tempo_import_flag_.test_and_set(std::memory_order_acquire))
      importTargetTempo();
    else if (!accel_import_flag_.test_and_set(std::memory_order_acquire))
      importAccel();
    else if (!meter_import_flag_.test_and_set(std::memory_order_acquire))
      importMeter();
    else if (!sound_high_import_flag_.test_and_set(std::memory_order_acquire))
      importSoundHigh();
    else if (!sound_mid_import_flag_.test_and_set(std::memory_order_acquire))
      importSoundMid();
    else if (!sound_low_import_flag_.test_and_set(std::memory_order_acquire))
      importSoundLow();
    else if (!audio_sink_import_flag_.test_and_set(std::memory_order_acquire))
      importAudioSink();
    
    if (frames_done_ >= frames_total_)
    {
      frames_left_=0;
      recalculateTempo();
      recalculateFramesTotal();
      frames_done_ = 0;
    }
    else
    {
      frames_left_ = frames_total_ - frames_done_;
    }

    recalculateLatency();
    
    exportCurrentTempo();
    exportCurrentAccel();
    exportLatency();
    exportNextAccent();
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
