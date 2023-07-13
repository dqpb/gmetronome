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
#include <array>
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
    static constexpr double kMinTempo = 30.0;
    static constexpr double kMaxTempo = 250.0;
    static constexpr double kMinAcceleration = 0.0;
    static constexpr double kMaxAcceleration = 1000.0;

    struct Statistics
    {
      microseconds  timestamp {0us};
      double        position {0.0};
      double        tempo {0.0};
      double        acceleration {0.0};
      int           n_beats {-1};
      int           n_accents {-1};
      int           next_accent {-1};
      microseconds  next_accent_delay {0us};
      int           generator_state {-1};
      microseconds  backend_latency {0us};
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
    void resetMeter();
    void setSound(Accent accent, const SoundParameters& params);

    void synchronize(double beat_dev, double tempo_dev);

    Ticker::Statistics getStatistics();

    bool hasStatistics() const;

  private:
    BeatStreamController stream_ctrl_;

    std::unique_ptr<Backend> backend_;
    std::unique_ptr<Backend> dummy_{nullptr};
    DeviceConfig actual_device_config_{kDefaultConfig};

    TickerState state_{0};

    std::atomic<double> in_tempo_{0.0};
    std::atomic<double> in_target_tempo_{0.0};
    std::atomic<double> in_accel_{0.0};
    Meter in_meter_{};
    double in_beat_dev_{0.0};
    double in_tempo_dev_{0.0};
    bool reset_meter_{false};
    std::array<SoundParameters, kNumAccents> in_sounds_;

    Ticker::Statistics out_stats_;
    bool has_stats_{false};

    std::atomic_flag tempo_imported_flag_;
    std::atomic_flag target_tempo_imported_flag_;
    std::atomic_flag accel_imported_flag_;
    std::atomic_flag meter_imported_flag_;
    std::atomic_flag sync_imported_flag_;
    std::array<std::atomic_flag, kNumAccents> sound_imported_flags_;
    std::atomic_flag sync_swap_backend_flag_;

    mutable std::mutex std_mutex_;
    mutable SpinLock spin_mutex_;

    bool importTempo();
    bool importTargetTempo();
    bool importAccel();
    bool importMeter();
    bool importSync();
    bool importSound(Accent accent);
    bool syncSwapBackend();
    void hardSwapBackend(std::unique_ptr<Backend>& backend);

    void importGeneratorSettings();
    bool importBackend();

    void exportStatistics();
    void exportBackend();

    void openBackend();
    void closeBackend();
    void startBackend();
    void stopBackend();
    void writeBackend(const void* data, size_t bytes);

    std::unique_ptr<std::thread> audio_thread_{nullptr};
    std::atomic<bool> stop_audio_thread_flag_{true};
    std::atomic<bool> audio_thread_finished_flag_{false};
    std::exception_ptr audio_thread_error_{nullptr};
    std::atomic<bool> audio_thread_error_flag_{false};
    std::condition_variable_any cond_var_;
    bool using_dummy_{true};
    bool ready_to_swap_{false};
    bool backend_swapped_{false};

    void startAudioThread();
    void stopAudioThread(bool join = false, const milliseconds& join_timeout = 2s);
    void audioThreadFunction() noexcept;
  };

}//namespace audio
#endif//GMetronome_Ticker_h
