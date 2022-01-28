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
    constexpr microseconds  kMaxChunkDuration   = 80ms;
    constexpr microseconds  kAvgChunkDuration   = 50ms;
    constexpr microseconds  kSineDuration       = 60ms;
    constexpr microseconds  kSwapBackendTimeout = 1s;

    template<class T>
    using Range = std::array<T,2>;

    const std::size_t kRangeMinIndex = 0;
    const std::size_t kRangeMaxIndex = 1;

    /** Clamp values to the inclusive range of min and max. */
    template<class T>
    T clamp(T val, const Range<T>& range)
    { return std::min( std::max(val, range[kRangeMinIndex]), range[kRangeMaxIndex] ); }

    constexpr Range<double> kTempoRange       = {30,250};
    constexpr Range<double> kAccelRange       = {0,1000};
  }

  // Ticker
  Ticker::Ticker()
    : generator_(kDefaultSpec, kMaxChunkDuration, kAvgChunkDuration),
      backend_ {createBackend(settings::kAudioBackendNone)},
      dummy_ {nullptr},
      device_config_ {kDefaultConfig},
      actual_device_config_ {kDefaultConfig},
      state_ {0},
      stop_audio_thread_flag_ {true},
      audio_thread_error_ {nullptr},
      audio_thread_error_flag_ {false},
      using_dummy_ {true},
      ready_to_swap_ {false},
      backend_swapped_ {false}
  {
    backend_ = createBackend(settings::kAudioBackendNone); // dummy backend

    tempo_imported_flag_.test_and_set();
    target_tempo_imported_flag_.test_and_set();
    accel_imported_flag_.test_and_set();
    meter_imported_flag_.test_and_set();
    sound_strong_imported_flag_.test_and_set();
    sound_mid_imported_flag_.test_and_set();
    sound_weak_imported_flag_.test_and_set();
    sync_swap_backend_flag_.test_and_set();
    device_config_imported_flag_.test_and_set();
  }

  Ticker::~Ticker()
  {
    reset();
  }

  std::unique_ptr<Backend> Ticker::getBackend(const microseconds& timeout)
  {
    std::unique_ptr<audio::Backend> tmp = nullptr;
    swapBackend(tmp, timeout);
    return tmp;
  }

  void Ticker::setBackend(std::unique_ptr<Backend> backend,
                          const microseconds& timeout)
  {
    swapBackend(backend, timeout);
    backend = nullptr;
  }

  void Ticker::swapBackend(std::unique_ptr<Backend>& backend,
                           const microseconds& timeout)
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
      sync_swap_backend_flag_.clear(std::memory_order_release);

      // wait for the audio thread to be ready
      ready_to_swap_ = false;
      bool success =  cond_var_.wait_for( lck, timeout, [&] { return ready_to_swap_; });

      if (success)
      {
        // swap and notify the audio thread
        std::swap(backend, backend_);
        backend_swapped_ = true;
        lck.unlock();
        cond_var_.notify_one();
      }
      else
      {
        // audio thread did not respond (no swap)
        sync_swap_backend_flag_.test_and_set();
        throw GMetronomeError {"failed to swap audio backend (audio thread not responding)"};
      }
    }
    else
    {
      sync_swap_backend_flag_.test_and_set();
      std::swap(backend, backend_);
    }
  }

  void Ticker::configureAudioDevice(const audio::DeviceConfig& config)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_device_config_ = config;
    }
    device_config_imported_flag_.clear(std::memory_order_release);
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

  bool Ticker::syncSwapBackend()
  {
    std::unique_lock<SpinLock> lck(spin_mutex_);

    backend_swapped_ = false;

    if (using_dummy_) // hide dummy backend from client
    {
      std::swap(dummy_, backend_);
      using_dummy_ = false;
    }

    // signal the client, that we are ready to swap the backends
    ready_to_swap_ = true;
    lck.unlock();
    cond_var_.notify_one();

    // wait for the new backend
    lck.lock();
    bool success = cond_var_.wait_for(lck, kSwapBackendTimeout,
                                      [&] {return backend_swapped_;});
    lck.unlock();

    // check the (possibly) new backend and (re-)install
    // the dummy backend if necessary
    if (!backend_)
    {
      backend_ = std::move(dummy_);
      using_dummy_ = true;
    }

    return success;
  }

  void Ticker::importDeviceConfig()
  {
    {
      std::lock_guard<SpinLock> lck(spin_mutex_);
      device_config_ = in_device_config_;
    }
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

    bool need_restart_backend = false;

    if (!device_config_imported_flag_.test_and_set(std::memory_order_acquire))
    {
      closeBackend();
      importDeviceConfig();
      configureBackend();
      need_restart_backend = true;
    }

    if (!sync_swap_backend_flag_.test_and_set(std::memory_order_acquire))
    {
      closeBackend();
      syncSwapBackend();
      need_restart_backend = true;
    }

    if (need_restart_backend)
      startBackend();
  }

  void Ticker::exportStatistics()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      out_stats_.timestamp = microseconds(g_get_monotonic_time());

      const auto& gen_stats = generator_.getStatistics();

      out_stats_.current_tempo = gen_stats.current_tempo;
      out_stats_.current_accel = gen_stats.current_accel;
      out_stats_.current_beat = gen_stats.current_beat;
      out_stats_.next_accent = gen_stats.next_accent;
      out_stats_.next_accent_delay = gen_stats.next_accent_delay;

      if (backend_)
        out_stats_.backend_latency = backend_->latency();
      else
        out_stats_.backend_latency = 0us;
    }
  }

  void Ticker::startBackend()
  {
    assert (backend_ != nullptr);
    switch (backend_->state())
    {
    case BackendState::kConfig:
      actual_device_config_ = backend_->open();
      [[fallthrough]];
    case BackendState::kOpen:
      backend_->start();
      [[fallthrough]];
    case BackendState::kRunning:
      // do nothing
    default:
      break;
    }
  }

  void Ticker::stopBackend()
  {
    assert (backend_ != nullptr);
    if (backend_)
    {
      switch (backend_->state())
      {
      case BackendState::kRunning:
        backend_->stop();
        [[fallthrough]];
      case BackendState::kOpen:
        [[fallthrough]];
      case BackendState::kConfig:
        // do nothing
      default:
        break;
      }
    }
  }

  void Ticker::configureBackend()
  {
    assert (backend_ != nullptr);
    closeBackend();
    backend_->configure(in_device_config_);
  }

  void Ticker::writeBackend(const void* data, size_t bytes)
  {
    if (backend_ && bytes > 0)
      backend_->write(data, bytes);
  }

  void Ticker::closeBackend()
  {
    assert (backend_ != nullptr);
    if (backend_)
    {
      switch (backend_->state())
      {
      case BackendState::kRunning:
        backend_->stop();
        [[fallthrough]];
      case BackendState::kOpen:
        backend_->close();
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
      startBackend();
      exportStatistics();
      writeBackend(data, bytes);

      // enter the main loop
      while ( ! stop_audio_thread_flag_ )
      {
        importSettings();
        generator_.cycle(data, bytes);
        writeBackend(data, bytes);
        exportStatistics();
      }

      generator_.stop(data, bytes);
      writeBackend(data, bytes);
      exportStatistics();
      stopBackend();
    }
    catch(...)
    {
      try { closeBackend(); }
      catch(...)
      {};
#ifndef NDEBUG
      std::cerr << "Ticker: error in audio thread" << std::endl;
#endif
      audio_thread_error_ = std::current_exception();
      audio_thread_error_flag_.store(true, std::memory_order_release);
    }
  }

}//namespace audio
