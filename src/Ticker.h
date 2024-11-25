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

  class Ticker {
  public:
    enum StateFlag
    {
      kStarted      = 0,
      kRunning      = 1,
      kError        = 2
    };

    using State = std::bitset<16>;

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

    static constexpr microseconds kDefaultSyncTime = 1s;

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

    Ticker::State state() const noexcept;

    void setTempo(double tempo);

    /**
     * @brief Accelerate the metronome continuously towards a target tempo
     *
     * @param accel  Magnitude of acceleration in BPM per minute
     * @param target Target tempo in BPM
     */
    void accelerate(double accel, double target);

    /**
     * @brief Accelerate the metronome stepwise towards a target tempo
     *
     * @param hold   Number of beats to hold the tempo
     * @param step   Magnitude of tempo change in BPM
     * @param target Target tempo in BPM
     */
    void accelerate(int hold, double step, double target);

    /**
     * @brief Stop an ongoing acceleration and reset tempo
     *
     * This function stops an ongoing continuous or stepwise acceleration that was
     * previously initiated by a call to one of the @ref accelerate overloads and
     * resets the tempo back to the last value set with the @ref setTempo function.
     *
     * A possible ongoing synchronization (see @ref synchronize) will be regarded
     * as obsolete and aborted as well.
     */
    void stopAcceleration();

    void synchronize(double beats, double tempo, microseconds time = kDefaultSyncTime);

    void setMeter(Meter meter);
    void resetMeter();
    void setSound(Accent accent, const SoundParameters& params);

    Ticker::Statistics getStatistics();

    bool hasStatistics() const;

  private:
    BeatStreamController stream_ctrl_;

    std::unique_ptr<Backend> backend_;
    std::unique_ptr<Backend> dummy_{nullptr};
    DeviceConfig actual_device_config_{kDefaultConfig};

    Ticker::State state_{0};

    // tempo
    std::atomic<double> in_tempo_{0.0};

    // acceleration
    AccelerationMode in_accel_mode_{AccelerationMode::kNoAcceleration};
    double in_target_{0.0};
    double in_accel_{0.0};
    int    in_hold_{0};
    double in_step_{0.0};

    // synchronization
    double in_beat_dev_{0.0};
    double in_tempo_dev_{0.0};
    microseconds in_sync_time_{0};

    // meter
    Meter in_meter_{};
    bool reset_meter_{false};

    // sound
    std::array<SoundParameters, kNumAccents> in_sounds_;

    Ticker::Statistics out_stats_;
    bool has_stats_{false};

    std::atomic_flag tempo_imported_flag_;
    std::atomic_flag accel_imported_flag_;
    std::atomic_flag sync_imported_flag_;
    std::atomic_flag meter_imported_flag_;
    std::array<std::atomic_flag, kNumAccents> sound_imported_flags_;
    std::atomic_flag swap_backend_flag_;

    mutable SpinLock spin_mutex_;

    bool importTempo();
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
