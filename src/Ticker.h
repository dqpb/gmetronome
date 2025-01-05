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

    enum class AccelMode
    {
      kNoAccel,
      kContinuous,
      kStepwise
    };

    struct Statistics
    {
      microseconds  timestamp {0us};

      AccelMode     mode {AccelMode::kNoAccel};
      bool          pending {false};
      bool          syncing {false};

      double        position {0.0};
      double        tempo {0.0};
      double        acceleration {0.0};
      double        target {0.0};

      int           hold {0};

      bool          default_meter {true};
      int           beats {-1};
      int           division {-1};
      int           accent {-1};
      microseconds  next_accent_delay {0us};

      GeneratorId   generator {kInvalidGenerator};
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

    /**
     * @brief Set the tempo of the metronome
     */
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
     * @brief Stop an ongoing acceleration
     *
     * This function ends an acceleration mode that was previously started
     * by a call to one of the @ref accelerate overloads and switches back
     * to the unaccelerated state.
     */
    void stopAcceleration();

    /**
     * @brief Synchronize the metronome with another oscillation
     * @see physics::BeatKinematics::synchronize for details
     */
    void synchronize(double beats, double tempo, microseconds time = kDefaultSyncTime);

    void setMeter(Meter meter);
    void resetMeter();
    void setSound(Accent accent, const SoundParameters& params);

    Ticker::Statistics getStatistics() const;
    Ticker::Statistics getStatistics(bool consume = true);

    bool hasStatistics() const;

  private:
    BeatStreamController stream_ctrl_;

    std::unique_ptr<Backend> backend_;
    std::unique_ptr<Backend> dummy_{nullptr};
    DeviceConfig actual_device_config_{kDefaultConfig};

    Ticker::State state_{0};

    // tempo
    double in_tempo_{0.0};

    // acceleration
    double in_target_{0.0};
    double in_accel_{0.0};
    int    in_hold_{0};
    double in_step_{0.0};

    // synchronization
    double in_sync_beats_{0.0};
    double in_sync_tempo_{0.0};
    microseconds in_sync_time_{0};

    // meter
    Meter in_meter_{};

    // sound
    std::array<SoundParameters, kNumAccents> in_sounds_;

    // input operations
    enum OpFlag
    {
      kOpFlagTempo       = 0,
      kOpFlagAccelCS     = 1,
      kOpFlagAccelSW     = 2,
      kOpFlagAccelSP     = 3,
      kOpFlagSync        = 4,
      kOpFlagMeter       = 5,
      kOpFlagMeterReset  = 6,
      kOpFlagSoundOff    = 7,
      kOpFlagSoundWeak   = 8,
      kOpFlagSoundMid    = 9,
      kOpFlagSoundStrong = 10,
      kNumOpFlags
    };

    using OpFlags = std::bitset<kNumOpFlags>;

    static constexpr OpFlags kOpMaskMeter {   0b11u << kOpFlagMeter};
    static constexpr OpFlags kOpMaskAccel {  0b111u << kOpFlagAccelCS};
    static constexpr OpFlags kOpMaskSound { 0b1111u << kOpFlagSoundOff};

    OpFlags in_ops_{0};

    std::atomic_flag swap_backend_flag_;
    mutable SpinLock spin_mutex_;

    Ticker::Statistics out_stats_;
    bool has_stats_{false};

    void openBackend();
    void closeBackend();
    void startBackend();
    void stopBackend();
    void writeBackend(const void* data, size_t bytes);

    bool syncSwapBackend();
    void hardSwapBackend(std::unique_ptr<Backend>& backend);

    bool importBackend();

    // current accel mode
    AccelMode accel_mode_{AccelMode::kNoAccel};

    // accel mode suspension handling
    static constexpr microseconds kDefaultAccelDeferTime = 2s;

    StreamTimer accel_defer_timer_;

    void deferAccel(microseconds time = kDefaultAccelDeferTime);
    bool tryAmendAccel(bool force = false);
    void abortAccelDefer();
    bool isAccelDeferred() const
      { return accel_defer_timer_.running(); }
    void updateAccelDeferTimer(size_t bytes)
      { accel_defer_timer_.step(bytes); }
    bool isAccelDeferExpired() const
      { return accel_defer_timer_.finished(); }

    void importTempo();
    void importAccelMode();
    void importAccelModeParams();
    void importSync();
    void importMeter();
    void importSound();
    void importSettingsInitial();
    bool tryImportSettings(bool force = false);

    bool tryExportStatistics(bool force = false);

    std::unique_ptr<std::thread> audio_thread_{nullptr};
    std::atomic_flag continue_audio_thread_flag_;
    std::atomic<bool> audio_thread_finished_flag_{true};
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
