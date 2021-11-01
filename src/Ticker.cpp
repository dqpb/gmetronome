/*
 * Copyright (C) 2020 The GMetronome Team
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
#include <mutex>
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
    static constexpr Range<double> kAccelRange       = {1,1000};
  }
  
  // Ticker
  Ticker::Ticker()
    : generator_(kDefaultSpec, kMaxChunkDuration, kAvgChunkDuration),
      audio_backend_(nullptr),
      state_(TickerState::kReady),
      except_(nullptr)
  {}
  
  Ticker::~Ticker()
  {
    stop();
  }  
  
  void Ticker::setAudioBackend(std::unique_ptr<AbstractAudioSink> new_backend)
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
  
  void Ticker::setSoundStrong(double frequency, double volume)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_strong_ = generateSound( frequency, volume, kDefaultSpec, kSineDuration );
    }
    sound_strong_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setSoundMid(double frequency, double volume)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_mid_ = generateSound( frequency, volume, kDefaultSpec, kSineDuration );
    }
    sound_mid_imported_flag_.clear(std::memory_order_release);
  }
  
  void Ticker::setSoundWeak(double frequency, double volume)
  {
    {
      std::lock_guard<SpinLock> guard(mutex_);
      in_sound_weak_ = generateSound( frequency, volume, kDefaultSpec, kSineDuration );
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

    if (state == TickerState::kPlaying) 
      return;
    if (state == TickerState::kError)
      return;
    
    state_ = TickerState::kPlaying;
    startWorker();
  }
    
  void Ticker::stop()
  {
    TickerState state = state_.load();
    if (state == TickerState::kError)
      return;
    
    stopWorker();
  }
    
  TickerState Ticker::state() const noexcept
  {
    return state_.load();
  }

  void Ticker::reset()
  {
    if (except_ != nullptr) {
      try {
	std::rethrow_exception(except_);
      }
      catch (...) {
	// handle error
      }
    }
    stopWorker();
  }
    
  void Ticker::startWorker() noexcept
  {
    if (worker_thread_ != nullptr)
      return;
    
    try {
      
      worker_thread_ = std::make_unique<std::thread>(&Ticker::worker, this);

    } catch(...) {}
  }
  
  void Ticker::stopWorker() noexcept
  {
    if (!worker_thread_)
      return;
    
    state_ = TickerState::kReady;
    try { worker_thread_->join(); } catch(...) {}
    worker_thread_.reset();  
    except_ = nullptr;
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
      stopAudioBackend();
      swap(audio_backend_, in_audio_backend_);
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
      case BackendState::kOpen:
        audio_backend_->start();
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
        audio_backend_->drain();
        audio_backend_->stop();
      case BackendState::kOpen:
        audio_backend_->close();
      case BackendState::kConfig:
        // do nothing
      default:
        break;
      }
    }
  }

  void Ticker::worker() noexcept
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
      except_ = std::current_exception();
      state_ = TickerState::kError;
    }
  }
    
}//namespace audio
