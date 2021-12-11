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
#include <array>
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
      state_(0),
      stop_audio_thread_flag_(true),
      audio_thread_error_(nullptr),
      audio_thread_error_flag_(false)
  {}

  Ticker::~Ticker()
  {
    reset();
  }

  std::unique_ptr<Backend> Ticker::getAudioBackend()
  {
    std::unique_ptr<audio::Backend> tmp = nullptr;
    swapAudioBackend(tmp);
    return tmp;
  }

  void Ticker::setAudioBackend(std::unique_ptr<Backend> backend)
  {
    swapAudioBackend(backend);
  }

  void Ticker::swapAudioBackend(std::unique_ptr<Backend>& backend)
  {
    auto cur_state = state();

    if ( cur_state.test(TickerStateFlag::kRunning)
         && ( ! cur_state.test(TickerStateFlag::kStarted)
              || cur_state.test(TickerStateFlag::kError) ) )
    {
      stopAudioThread(true);
    }

    if ( state().test(TickerStateFlag::kRunning) )
    {
      std::unique_lock<SpinLock> lck(spin_mutex_);

      // initiate backend swap
      std::cout << "M: initiate backend swap" << std::endl;
      audio_backend_imported_flag_.clear(std::memory_order_release);

      // wait for the audio thread to be ready
      ready_to_swap_ = false;
      std::cout << "M: wait for audio thread to be ready" << std::endl;
      bool success =  cond_var_.wait_for( lck, 500ms, [&] { return ready_to_swap_; });

      if (success)
      {
        // swap and notify the audio thread
        std::cout << "M: swap and notify audio thread" << std::endl;
        std::swap(audio_backend_, backend);
        audio_backend_swapped_ = true;
        lck.unlock();
        cond_var_.notify_one();
      }
      else
      {
        // audio thread did not respond (no swap)
        std::cout << "M: audio thread did not respond (no swap)" << std::endl;
        audio_backend_imported_flag_.test_and_set();
        throw GMetronomeError {"failed to swap audio backend (audio thread not responding)"};
      }
    }
    else
    {
      audio_backend_imported_flag_.test_and_set();
      std::swap(audio_backend_, backend);
    }
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
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_meter_ = std::move(meter);
    }
    meter_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundStrong(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_strong_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_strong_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundMid(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_mid_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_mid_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundWeak(double frequency, double volume, double balance)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_weak_ = generateSound( frequency, volume, balance, kDefaultSpec, kSineDuration );
    }
    sound_weak_imported_flag_.clear(std::memory_order_release);
  }

  Ticker::Statistics Ticker::getStatistics() const
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      return out_stats_;
    }
  }

  void Ticker::start()
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    auto current_state = state();

    if (current_state.test(TickerStateFlag::kError))
    {
      assert(audio_thread_error_ != nullptr);
      std::rethrow_exception(audio_thread_error_);
    }

    if (current_state.test(TickerStateFlag::kRunning))
      stopAudioThread(true);

    startAudioThread();

    state_.set(TickerStateFlag::kStarted);
  }

  void Ticker::stop()
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    auto current_state = state();

    if (current_state.test(TickerStateFlag::kError))
    {
      assert(audio_thread_error_ != nullptr);
      std::rethrow_exception(audio_thread_error_);
    }

    if (current_state.test(TickerStateFlag::kRunning))
      stopAudioThread();

    state_.reset(TickerStateFlag::kStarted);
  }

  void Ticker::reset() noexcept
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    try {
      if (audio_thread_) stopAudioThread(true);

      state_.reset();
      audio_thread_error_ = nullptr;
      audio_thread_error_flag_.store(false, std::memory_order_release);
    }
    catch (...) {
      // we can't stop the audio thread, so we keep the current state
    }
  }

  TickerState Ticker::state() const noexcept
  {
    TickerState out_state = state_;

    if (audio_thread_error_flag_.load(std::memory_order_acquire))
      out_state.set(TickerStateFlag::kError);

    return out_state;
  }

  void Ticker::startAudioThread()
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    assert( audio_thread_ == nullptr );

    stop_audio_thread_flag_ = false;

    try {
      state_.set(TickerStateFlag::kRunning);
      audio_thread_ = std::make_unique<std::thread>(&Ticker::audioThreadFunction, this);
    }
    catch(...)
    {
      state_.reset(TickerStateFlag::kRunning);
      stop_audio_thread_flag_ = true;
      throw GMetronomeError("failed to start audio thread");
    }
  }

  void Ticker::stopAudioThread(bool join)
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    assert( audio_thread_ != nullptr );

    stop_audio_thread_flag_ = true;

    if (join && audio_thread_->joinable())
    {
      try {
        audio_thread_->join();
        audio_thread_.reset(); //noexcept
      }
      catch(...)
      {
        throw GMetronomeError("failed to join audio thread");
      }
      state_.reset(TickerStateFlag::kRunning);
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
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapMeter(in_meter_);
    }
    else meter_imported_flag_.clear();
  }

  void Ticker::importSoundStrong()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundStrong(in_sound_strong_);
    }
    else sound_strong_imported_flag_.clear();
  }

  void Ticker::importSoundMid()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundMid(in_sound_mid_);
    }
    else sound_mid_imported_flag_.clear();
  }

  void Ticker::importSoundWeak()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      generator_.swapSoundWeak(in_sound_weak_);
    }
    else sound_weak_imported_flag_.clear();
  }

  void Ticker::importAudioBackend()
  {
    // first, we close the old backend
    closeAudioBackend();

    std::unique_lock<SpinLock> lck(spin_mutex_);

    audio_backend_swapped_ = false;

    // signal, that we are ready to swap the backends
    std::cout << "T: ready to swap" << std::endl;
    ready_to_swap_ = true;
    lck.unlock();
    cond_var_.notify_one();

    // wait for the new backend
    lck.lock();
    std::cout << "T: waiting for the new backend" << std::endl;
    bool success = cond_var_.wait_for(lck, 500ms, [&] {return audio_backend_swapped_;});

    lck.unlock();

    if (success) {
      // swap succeeded: start new backend
      std::cout << "T: swap succeeded; start new audio backend" << std::endl;
      startAudioBackend();
    }
    else {
      // swap failed: will continue with the old backend
      std::cout << "T: swap failed; restart old audio backend" << std::endl;
      startAudioBackend();
    }
    std::cout << "T: backend started" << std::endl;
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
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      out_stats_.timestamp = microseconds(g_get_monotonic_time());

      if (audio_backend_)
        out_stats_.backend_latency = audio_backend_->latency();

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

  void Ticker::closeAudioBackend()
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
      startAudioBackend();
      exportStatistics();
      writeAudioBackend(data, bytes);

      // enter the main loop
      while ( ! stop_audio_thread_flag_ )
      {
        importSettings();
        generator_.cycle(data, bytes);
        writeAudioBackend(data, bytes);
        exportStatistics();
      }

      generator_.stop(data, bytes);
      writeAudioBackend(data, bytes);
      exportStatistics();
      closeAudioBackend();
    }
    catch(...)
    {
      try { closeAudioBackend(); }
      catch(...)
      {
        // the client might need to change the audio backend which
        // will forcefully release the old backend resource
      };
      std::cout << "ERROR in audio thread: set error_flag_" << std::endl;
      audio_thread_error_ = std::current_exception();
      audio_thread_error_flag_.store(true, std::memory_order_release);
    }
  }

}//namespace audio
