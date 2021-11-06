/*
 * Copyright (C) 2020, 2021 The GMetronome Team
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

#include "Ticker.h"
#include "Synthesizer.h"
#include "PulseAudio.h"
#include <mutex>
#include <cassert>
#include <iostream>

namespace audio {

  namespace
  {
    static constexpr microseconds  kMaxChunkDuration = 80ms;
    static constexpr microseconds  kAvgChunkDuration = 50ms;
    static constexpr double        kSineVolume       = 1.;
    static constexpr microseconds  kSineDuration     = 60ms;

    template<class T>
    using Range = std::array<T,2>;

    const std::size_t kRangeMinIndex = 0;
    const std::size_t kRangeMaxIndex = 1;
    
    /** Clamp values to the inclusive range of min and max. */
    template<class T>
    T clamp(T val, const Range<T>& range)
    { return std::min( std::max(val, range[kRangeMinIndex]), range[kRangeMaxIndex] ); }

    static constexpr Range<double> kTempoRange       = {30,250};
    static constexpr Range<double> kAccelRange       = {0,1000};
  }
  
  // Ticker
  Ticker::Ticker()
    : generator_(kDefaultSpec, kMaxChunkDuration, kAvgChunkDuration),
      audio_backend_(nullptr),
      state_(TickerState::kReady),
      audio_thread_error_(nullptr)
  {}
  
  Ticker::~Ticker()
  {
    reset();
  }  
  
  void Ticker::setAudioBackend(std::unique_ptr<Backend> new_backend)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_audio_backend_ = std::move(new_backend);
    }
    audio_backend_imported_flag_.clear(std::memory_order_release);
  }
    
  void Ticker::setTempo(double tempo)
  {
    in_tempo_.store( clamp(tempo, kTempoRange) );
    tempo_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setTargetTempo(double target_tempo)
  {
    in_target_tempo_.store( clamp(target_tempo, kTempoRange) );
    target_tempo_imported_flag_.clear(std::memory_order_release);
  }
 
  void Ticker::setAccel(double accel)
  {
    in_accel_.store( clamp(accel, kAccelRange) );
    accel_imported_flag_.clear(std::memory_order_release);
  }
 
  void Ticker::setMeter(Meter meter)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_meter_ = std::move(meter);
    }
    meter_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setSoundStrong(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_strong_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_strong_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setSoundMid(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_mid_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_mid_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setSoundWeak(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_weak_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_weak_imported_flag_.clear(std::memory_order_release);
  }

  Ticker::Statistics Ticker::getStatistics() const
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      return out_stats_;
    }
  }
    
  void Ticker::start()
  {
    TickerState state = state_.load();

    switch ( state ) {

    case TickerState::kReady:
      startAudioThread();
      break;
      
    case TickerState::kPlaying:
      // nothing to do
      break;
      
    case TickerState::kError:
      std::rethrow_exception(audio_thread_error_);
      break;
      
    default:
      break;
    }
  }
    
  void Ticker::stop()
  {
    TickerState state = state_.load();
    switch ( state ) {
      
    case TickerState::kReady:
      break;
      
    case TickerState::kPlaying:
      stopAudioThread();
      break;
      
    case TickerState::kError:
      assert(audio_thread_ != nullptr);
      assert(audio_thread_error_ != nullptr);      
      std::rethrow_exception(audio_thread_error_);
      break;
      
    default:
      break;
    }
  }

  void Ticker::reset() noexcept
  {
    try {      
      if (audio_thread_)
        stopAudioThread();
      
      state_ = TickerState::kReady;
    }
    catch (...) {
      // we can't stop the audio thread, so we keep the current state,
      // which should be TickerState::kPlaying or TickerState::kError
    }
  }
  
  TickerState Ticker::state() const noexcept
  {
    return state_.load();
  }
  
  void Ticker::startAudioThread()
  {
    assert( audio_thread_ == nullptr );
    assert( state_ == TickerState::kReady );

    TickerState old_state = state_;

    try {      
      state_ = TickerState::kPlaying;
      audio_thread_ =
        std::make_unique<std::thread>(&Ticker::audioThreadFunction, this);
    } catch(...) {
      state_ = old_state;
      throw GMetronomeError("couldn't start audio thread");
    }
  }
  
  void Ticker::stopAudioThread()
  {
    assert( audio_thread_ != nullptr );
    assert( state_ == TickerState::kPlaying || state_ == TickerState::kError );

    TickerState old_state = state_;
    
    try {
      state_ = TickerState::kReady;
      audio_thread_->join();
      audio_thread_.reset();
    } catch(...) {
      state_ = old_state; 
      throw GMetronomeError("couldn't stop audio thread");
    }
  }
  
  void Ticker::importTempo()
  { generator_.setTempo(in_tempo_.load()); }
  
  void Ticker::importTargetTempo()
  { generator_.setTargetTempo(in_target_tempo_.load()); }
  
  void Ticker::importAccel()
  { generator_.setAccel(in_accel_.load()); }

  void Ticker::importMeter()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapMeter(in_meter_);
    }
    else meter_imported_flag_.clear();
  }

  void Ticker::importSoundStrong()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundStrong(in_sound_strong_);
    }
    else sound_strong_imported_flag_.clear();
  }

  void Ticker::importSoundMid()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundMid(in_sound_mid_);
    }
    else sound_mid_imported_flag_.clear();
  }

  void Ticker::importSoundWeak()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundWeak(in_sound_weak_);
    }
    else sound_weak_imported_flag_.clear();
  }
  
  void Ticker::importAudioBackend()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      try { stopAudioBackend(); }
      catch(...)
      {
        // if we can't stop the old backend, we will continue anyway
        // and release the old backend resource
      }
      std::swap(audio_backend_, in_audio_backend_);
      
      // since we switch the audio backend, we have no realtime
      // contraints here and can safely release the old backend 
      in_audio_backend_ = nullptr;
      
      startAudioBackend();
    }
    else audio_backend_imported_flag_.clear();
  }

  void Ticker::importSettings()
  {
    if (!tempo_imported_flag_.test_and_set(std::memory_order_acquire))
      importTempo();
    if (!target_tempo_imported_flag_.test_and_set(std::memory_order_acquire))
      importTargetTempo();
    if (!accel_imported_flag_.test_and_set(std::memory_order_acquire))
      importAccel();
    if (!meter_imported_flag_.test_and_set(std::memory_order_acquire))
      importMeter();
    if (!sound_strong_imported_flag_.test_and_set(std::memory_order_acquire))
      importSoundStrong();
    if (!sound_mid_imported_flag_.test_and_set(std::memory_order_acquire))
      importSoundMid();
    if (!sound_weak_imported_flag_.test_and_set(std::memory_order_acquire))
      importSoundWeak();
    if (!audio_backend_imported_flag_.test_and_set(std::memory_order_acquire))
      importAudioBackend();
  }

  void Ticker::exportStatistics()
  {
    if (std::unique_lock<SpinLock> lck(mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      out_stats_.timestamp = microseconds(g_get_monotonic_time());
      
      if (audio_backend_)
        out_stats_.backend_latency = microseconds( audio_backend_->latency() );
      
      out_stats_.generator = generator_.getStatistics();
    }
  }
  
  void Ticker::startAudioBackend()
  {
    if (audio_backend_)
    {
      switch (audio_backend_->state())
      {
      case BackendState::kConfig:
        audio_backend_->open();
        [[fallthrough]];
      case BackendState::kOpen:
        audio_backend_->start();
        [[fallthrough]];
      case BackendState::kRunning:
        // do nothing
      default:
        break;
      }
    }
  }
  
  void Ticker::writeAudioBackend(const void* data, size_t bytes)
  {
    if (audio_backend_ && bytes > 0)
      audio_backend_->write(data, bytes);
  }
  
  void Ticker::stopAudioBackend()
  {
    if (audio_backend_)
    {
      switch (audio_backend_->state())
      {
      case BackendState::kRunning:
        audio_backend_->stop();
        [[fallthrough]];
      case BackendState::kOpen:
        audio_backend_->close();
        [[fallthrough]];
      case BackendState::kConfig:
        // do nothing
      default:
        break;
      }
    }
  }
  
  void Ticker::audioThreadFunction() noexcept
  {
    const void* data;
    size_t bytes;
    
    try {
      importSettings();
      generator_.start(data, bytes);
      exportStatistics();
      startAudioBackend();
      writeAudioBackend(data, bytes);
      
      // enter the main loop
      while ( state_.load() != TickerState::kReady )
      {                
        importSettings();
        generator_.cycle(data, bytes);
        exportStatistics();
        writeAudioBackend(data, bytes);
      }
      
      generator_.stop(data, bytes);
      exportStatistics();
      writeAudioBackend(data, bytes);
      stopAudioBackend();
    }
    catch(...)
    {
      try { stopAudioBackend(); }
      catch(...)
      {
        // nop;
        // the client might need to change the audio backend which
        // will forcefully release the old backend resource in case
        // of an error during shutdown
      };
      audio_thread_error_ = std::current_exception();
      state_ = TickerState::kError;
    }
  }
    
}//namespace audio
