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

#ifndef GMetronome_Ticker_h
#define GMetronome_Ticker_h

#include "Generator.h"
#include "AudioBackend.h"
#include "AudioBuffer.h"
#include "Meter.h"
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
      microseconds timestamp;
      microseconds backend_latency;
      Generator::Statistics generator;
    };

  public:
    Ticker();    
    ~Ticker();
    
    void setBackend(std::unique_ptr<Backend> backend);

    std::unique_ptr<Backend> getBackend(const microseconds& timeout = 500ms);

    void swapBackend(std::unique_ptr<Backend>& backend,
                     const microseconds& timeout = 500ms);

    void configureAudioDevice(const audio::DeviceConfig& config);
    
    void start();
    void stop();
    void reset() noexcept;
    
    TickerState state() const noexcept;
    
    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void setMeter(Meter meter);
    
    void setSoundStrong(double frequency, double volume, double balance);
    void setSoundMid(double frequency, double volume, double balance);
    void setSoundWeak(double frequency, double volume, double balance);
    
    Ticker::Statistics getStatistics() const;
    
  private:
    Generator generator_;

    std::unique_ptr<Backend> backend_;
    DeviceConfig actual_device_config_;
    
    TickerState state_;
    
    std::atomic<double> in_tempo_;
    std::atomic<double> in_target_tempo_;
    std::atomic<double> in_accel_;
    Meter in_meter_;
    Buffer in_sound_strong_;
    Buffer in_sound_mid_;
    Buffer in_sound_weak_;
    std::unique_ptr<Backend> in_backend_;    
    audio::DeviceConfig in_device_config_;
    
    Ticker::Statistics out_stats_;
    
    std::atomic_flag tempo_imported_flag_;
    std::atomic_flag target_tempo_imported_flag_;
    std::atomic_flag accel_imported_flag_;
    std::atomic_flag meter_imported_flag_;
    std::atomic_flag sound_strong_imported_flag_;
    std::atomic_flag sound_mid_imported_flag_;
    std::atomic_flag sound_weak_imported_flag_;
    std::atomic_flag backend_imported_flag_;
    std::atomic_flag sync_swap_backend_flag_;
    std::atomic_flag device_config_imported_flag_;

    mutable std::mutex std_mutex_;
    mutable SpinLock spin_mutex_;
    
    void importTempo();
    void importTargetTempo();
    void importAccel();
    void importMeter();
    void importSoundStrong();
    void importSoundMid();
    void importSoundWeak();
    void importBackend();
    void importDeviceConfig();
    void syncSwapBackend();

    void importSettings();

    void exportStatistics();
    
    void startBackend();
    void writeBackend(const void* data, size_t bytes);
    void closeBackend();

    std::unique_ptr<std::thread> audio_thread_;
    std::atomic<bool> stop_audio_thread_flag_;
    std::exception_ptr audio_thread_error_;
    std::atomic<bool> audio_thread_error_flag_;
    std::condition_variable_any cond_var_;
    bool using_dummy_;
    bool ready_to_swap_;
    bool backend_swapped_;
    bool need_restart_backend_;
    
    void startAudioThread();
    void stopAudioThread(bool join = false);
    void audioThreadFunction() noexcept;
  };
  
}//namespace audio
#endif//GMetronome_Ticker_h
