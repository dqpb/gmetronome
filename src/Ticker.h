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

#ifndef GMetronome_Ticker_h
#define GMetronome_Ticker_h

#include "Meter.h"
#include "Synthesizer.h"
#include "Generator.h"
#include "AudioBackend.h"
#include "SpinLock.h"

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <bitset>

namespace audio {

  enum TickerStateFlag
  {
    kStarted      = 0,
    kRunning      = 1,
    kError        = 2
  };

  using TickerState = std::bitset<16>;

  class Ticker {
  public:
    struct Statistics
    {
      microseconds  timestamp;
      double        current_tempo;
      double        current_accel;
      double        current_beat;
      int           next_accent;
      microseconds  next_accent_delay;
      microseconds  backend_latency;
    };

  public:
    Ticker();
    ~Ticker();

    void setBackend(std::unique_ptr<Backend> backend,
                    const microseconds& timeout = 2s);

    std::unique_ptr<Backend> getBackend(const microseconds& timeout = 2s);

    void swapBackend(std::unique_ptr<Backend>& backend,
                     const microseconds& timeout = 2s);

    void start();
    void stop();
    void reset() noexcept;

    TickerState state() const noexcept;

    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void setMeter(Meter meter);

    void syncMeter(double beat);

    void setSoundStrong(const SoundParameters& params);
    void setSoundMid(const SoundParameters& params);
    void setSoundWeak(const SoundParameters& params);

    Ticker::Statistics getStatistics() const;

  private:
    Generator generator_;

    std::unique_ptr<Backend> backend_;
    std::unique_ptr<Backend> dummy_;
    DeviceConfig actual_device_config_;

    TickerState state_;

    std::atomic<double> in_tempo_;
    std::atomic<double> in_target_tempo_;
    std::atomic<double> in_accel_;
    Meter in_meter_;
    SoundParameters in_sound_strong_;
    SoundParameters in_sound_mid_;
    SoundParameters in_sound_weak_;

    Ticker::Statistics out_stats_;

    std::atomic_flag tempo_imported_flag_;
    std::atomic_flag target_tempo_imported_flag_;
    std::atomic_flag accel_imported_flag_;
    std::atomic_flag meter_imported_flag_;
    std::atomic_flag sound_strong_imported_flag_;
    std::atomic_flag sound_mid_imported_flag_;
    std::atomic_flag sound_weak_imported_flag_;
    std::atomic_flag sync_swap_backend_flag_;

    mutable std::mutex std_mutex_;
    mutable SpinLock spin_mutex_;

    bool importTempo();
    bool importTargetTempo();
    bool importAccel();
    bool importMeter();
    bool importSoundStrong();
    bool importSoundMid();
    bool importSoundWeak();
    bool syncSwapBackend();
    void hardSwapBackend(std::unique_ptr<Backend>& backend);

    bool importGeneratorSettings();
    bool importBackend();

    void exportStatistics();
    void exportBackend();

    void openBackend();
    void closeBackend();
    void startBackend();
    void stopBackend();
    void writeBackend(const void* data, size_t bytes);

    std::unique_ptr<std::thread> audio_thread_;
    std::atomic<bool> stop_audio_thread_flag_;
    std::atomic<bool> audio_thread_finished_flag_;
    std::exception_ptr audio_thread_error_;
    std::atomic<bool> audio_thread_error_flag_;
    std::condition_variable_any cond_var_;
    bool using_dummy_;
    bool ready_to_swap_;
    bool backend_swapped_;

    void startAudioThread();
    void stopAudioThread(bool join = false, const milliseconds& join_timeout = 2s);
    void audioThreadFunction() noexcept;
  };

}//namespace audio
#endif//GMetronome_Ticker_h
