/*
 * Copyright (C) 2020-2023 The GMetronome Team
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

#include "Ticker.h"
#include <algorithm>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  namespace {

    constexpr microseconds  kSwapBackendTimeout = 1s;

  }//unnamed namespace

  // Ticker
  Ticker::Ticker()
    : backend_ {createBackend(settings::kAudioBackendNone)}
  {
    tempo_imported_flag_.test_and_set();
    target_tempo_imported_flag_.test_and_set();
    accel_imported_flag_.test_and_set();
    meter_imported_flag_.test_and_set();
    beat_imported_flag_.test_and_set();
    sound_strong_imported_flag_.test_and_set();
    sound_mid_imported_flag_.test_and_set();
    sound_weak_imported_flag_.test_and_set();
    sync_swap_backend_flag_.test_and_set();
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
    // If the audio thread is still running after a stop() call or in the
    // error state, we explicitly join it and swap the audio backends directly,
    // otherwise we try to synchronize the threads with a conditional variable
    // to prevent data races during the swap operation.

    if (auto s = state();
        s.test(TickerStateFlag::kRunning)
        && ( ! s.test(TickerStateFlag::kStarted) || s.test(TickerStateFlag::kError) ) )
    {
      stopAudioThread(true); // join
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
    else // audio thread is not running
    {
      sync_swap_backend_flag_.test_and_set();

      if (backend_)
        closeBackend();

      hardSwapBackend(backend);
    }
  }

  void Ticker::setTempo(double tempo)
  {
    in_tempo_.store(std::clamp(tempo, kMinTempo, kMaxTempo));
    tempo_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setTargetTempo(double target_tempo)
  {
    in_target_tempo_.store(std::clamp(target_tempo, kMinTempo, kMaxTempo));
    target_tempo_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setAccel(double accel)
  {
    in_accel_.store(std::clamp(accel, kMinAcceleration, kMaxAcceleration));
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

  void Ticker::synchronize(double beat_dev, double tempo_dev)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_beat_dev_ = beat_dev;
      in_tempo_dev_ = tempo_dev;
    }
    beat_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundStrong(const SoundParameters& params)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_strong_ = params;
    }
    sound_strong_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundMid(const SoundParameters& params)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_mid_ = params;
    }
    sound_mid_imported_flag_.clear(std::memory_order_release);
  }

  void Ticker::setSoundWeak(const SoundParameters& params)
  {
    {
      std::lock_guard<SpinLock> guard(spin_mutex_);
      in_sound_weak_ = params;
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
      stopAudioThread(true); // join

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
      stopAudioThread(false); // do not join

    state_.reset(TickerStateFlag::kStarted);
  }

  void Ticker::reset() noexcept
  {
    try {
      if (audio_thread_) stopAudioThread(true); // join

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

    try {
      stop_audio_thread_flag_ = false;
      audio_thread_finished_flag_ = false;
      state_.set(TickerStateFlag::kRunning);

      audio_thread_ = std::make_unique<std::thread>(&Ticker::audioThreadFunction, this);
    }
    catch(...)
    {
      state_.reset(TickerStateFlag::kRunning);
      audio_thread_finished_flag_ = true;
      stop_audio_thread_flag_ = true;

      throw GMetronomeError("failed to start audio thread");
    }
  }

  void Ticker::stopAudioThread(bool join, const milliseconds& join_timeout)
  {
    assert( audio_thread_ != nullptr );

    stop_audio_thread_flag_ = true;

    if (join && audio_thread_->joinable())
    {
      // To prevent freezing of the ui thread in case of a non-responding audio thread
      // we wait for the audio_thread_finished_flag to be set by the audio thread just
      // before the end of execution. On success (i.e. no timeout) we join the audio thread,
      // otherwise we raise an exception.

      std::unique_lock<SpinLock> lck(spin_mutex_);

      bool success = cond_var_.wait_for(lck, join_timeout,
                                        [&] {return audio_thread_finished_flag_ == true;});
      lck.unlock();

      if (success)
      {
        try {
          audio_thread_->join();
          audio_thread_.reset(); //noexcept
        }
        catch(...)
        {
          throw GMetronomeError("Failed to join audio thread.");
        }
        state_.reset(TickerStateFlag::kRunning);
      }
      else throw GMetronomeError("Audio thread not responing. (Timeout)");
    }
  }

  bool Ticker::importTempo()
  {
    stream_ctrl_.setTempo(in_tempo_.load());
    return true;
  }

  bool Ticker::importTargetTempo()
  {
    stream_ctrl_.setTargetTempo(in_target_tempo_.load());
    return true;
  }

  bool Ticker::importAccel()
  {
    stream_ctrl_.setAcceleration(in_accel_.load());
    return true;
  }

  bool Ticker::importMeter()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      stream_ctrl_.swapMeter(in_meter_);
      return true;
    }
    else {
      return false;
    }
  }

  bool Ticker::importBeat()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      stream_ctrl_.synchronize(in_beat_dev_, in_tempo_dev_);
      return true;
    }
    else {
      return false;
    }
  }

  bool Ticker::importSoundStrong()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      stream_ctrl_.setSound(Accent::kAccentStrong, in_sound_strong_);
      return true;
    }
    else {
      return false;
    }
  }

  bool Ticker::importSoundMid()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      stream_ctrl_.setSound(Accent::kAccentMid, in_sound_mid_);
      return true;
    }
    else {
      return false;
    }
  }

  bool Ticker::importSoundWeak()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      stream_ctrl_.setSound(Accent::kAccentWeak, in_sound_weak_);
      return true;
    }
    else {
      return false;
    }
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

  void Ticker::hardSwapBackend(std::unique_ptr<Backend>& backend)
  {
    // same as syncSwapBackend() without thread synchronization

    if (using_dummy_)
    {
      std::swap(dummy_, backend_);
      using_dummy_ = false;
    }

    std::swap(backend, backend_);

    if (!backend_)
    {
      backend_ = std::move(dummy_);
      using_dummy_ = true;
    }
  }

  void Ticker::importGeneratorSettings()
  {
    if (!tempo_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importTempo()) tempo_imported_flag_.clear();

    if (!target_tempo_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importTargetTempo()) target_tempo_imported_flag_.clear();

    if (!accel_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importAccel()) accel_imported_flag_.clear();

    if (!meter_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importMeter()) meter_imported_flag_.clear();

    if (!beat_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importBeat()) beat_imported_flag_.clear();

    if (!sound_strong_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importSoundStrong()) sound_strong_imported_flag_.clear();

    if (!sound_mid_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importSoundMid()) sound_mid_imported_flag_.clear();

    if (!sound_weak_imported_flag_.test_and_set(std::memory_order_acquire))
      if (!importSoundWeak()) sound_weak_imported_flag_.clear();
  }

  bool Ticker::importBackend()
  {
    if (!sync_swap_backend_flag_.test_and_set(std::memory_order_acquire))
    {
      closeBackend();
      return syncSwapBackend();
    }
    else return false;
  }

  void Ticker::exportStatistics()
  {
    if (std::unique_lock<SpinLock> lck(spin_mutex_, std::try_to_lock);
        lck.owns_lock())
    {
      out_stats_.timestamp = microseconds(g_get_monotonic_time());

      const auto& gen_stats = stream_ctrl_.status();
      const auto& meter = stream_ctrl_.meter();

      out_stats_.position = gen_stats.position;
      out_stats_.tempo = gen_stats.tempo;
      out_stats_.acceleration = gen_stats.acceleration;
      out_stats_.n_beats = meter.beats();
      out_stats_.n_accents = meter.beats() * meter.division();
      out_stats_.next_accent = gen_stats.next_accent;
      out_stats_.next_accent_delay = gen_stats.next_accent_delay;
      out_stats_.generator_state = gen_stats.state;

      if (backend_)
        out_stats_.backend_latency = backend_->latency();
      else
        out_stats_.backend_latency = 0us;
    }
  }

  void Ticker::openBackend()
  {
    assert (backend_ != nullptr);
    switch (backend_->state())
    {
    case BackendState::kConfig:
      actual_device_config_ = backend_->open();
      if (actual_device_config_.spec.channels <= 0)
        throw GMetronomeError {"Unsupported audio device (invalid number of channels)"};
      if (actual_device_config_.spec.rate <= 0)
        throw GMetronomeError {"Unsupported audio device (invalid sample rate)"};
      break;
    case BackendState::kOpen:
      [[fallthrough]];
    case BackendState::kRunning:
      [[fallthrough]];
    default:
      // nothing
      break;
    }
  }

  void Ticker::closeBackend()
  {
    assert (backend_ != nullptr);
    switch (backend_->state())
    {
    case BackendState::kRunning:
      stopBackend();
      [[fallthrough]];
    case BackendState::kOpen:
      backend_->close();
      break;
    case BackendState::kConfig:
      [[fallthrough]];
    default:
      // nothing
      break;
    }
  }

  void Ticker::startBackend()
  {
    assert (backend_ != nullptr);
    switch (backend_->state())
    {
    case BackendState::kConfig:
      openBackend();
      [[fallthrough]];
    case BackendState::kOpen:
      backend_->start();
      break;
    case BackendState::kRunning:
      [[fallthrough]];
    default:
      // nothing
      break;
    }
  }

  void Ticker::stopBackend()
  {
    assert (backend_ != nullptr);
    switch (backend_->state())
    {
    case BackendState::kRunning:
      backend_->stop();
      break;
    case BackendState::kOpen:
      [[fallthrough]];
    case BackendState::kConfig:
      [[fallthrough]];
    default:
      // nothing
      break;
    }
  }

  void Ticker::writeBackend(const void* data, size_t bytes)
  {
    assert(backend_ != nullptr);
    if (bytes > 0)
      backend_->write(data, bytes);
  }

  void Ticker::audioThreadFunction() noexcept
  {
    const void* data;
    size_t bytes;

    try {
      openBackend(); // sets actual_device_config_
      stream_ctrl_.prepare(actual_device_config_.spec);
      importGeneratorSettings();
      stream_ctrl_.start(kFillBufferGenerator);
      exportStatistics();
      startBackend();

      // enter the main loop
      while ( ! stop_audio_thread_flag_ )
      {
        if (importBackend())
        {
          openBackend(); // updates actual_device_config_
          stream_ctrl_.prepare(actual_device_config_.spec);
          startBackend();
        }
        importGeneratorSettings();
        stream_ctrl_.cycle(data, bytes);
        writeBackend(data, bytes);
        exportStatistics();
      }

      stream_ctrl_.stop();
      exportStatistics();
      stopBackend();
    }
    catch(...)
    {
#ifndef NDEBUG
      std::cerr << "Ticker: error in audio thread" << std::endl;
#endif
      try { closeBackend(); }
      catch(...)
      {
#ifndef NDEBUG
      std::cerr << "Ticker: failed to close audio backend after error in audio thread"
                << std::endl;
#endif
      };
      audio_thread_error_ = std::current_exception();
      audio_thread_error_flag_.store(true, std::memory_order_release);
    }

    audio_thread_finished_flag_ = true;
    cond_var_.notify_one();
  }

}//namespace audio
