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

#include <memory>
#include <thread>
#include <atomic>

#include "Generator.h"
#include "AudioBackend.h"
#include "AudioBuffer.h"
#include "Meter.h"
#include "SpinLock.h"

namespace audio {
  
  enum class TickerState
  {
    kReady,
    kPlaying,
    kError
  };
    
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
    
    void setAudioBackend(std::unique_ptr<Backend> backend);
    
    void start();
    void stop();
    void reset() noexcept;
    
    TickerState state() const noexcept;
    
    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void setMeter(Meter meter);
    
    void setSoundStrong(double frequency, double volume);
    void setSoundMid(double frequency, double volume);
    void setSoundWeak(double frequency, double volume);
    
    Ticker::Statistics getStatistics() const;
    
  private:
    Generator generator_;

    std::unique_ptr<Backend> audio_backend_;
    
    std::atomic<TickerState> state_;
    
    std::exception_ptr audio_thread_error_;
    
    std::atomic<double> in_tempo_;
    std::atomic<double> in_target_tempo_;
    std::atomic<double> in_accel_;
    Meter in_meter_;
    Buffer in_sound_strong_;
    Buffer in_sound_mid_;
    Buffer in_sound_weak_;
    std::unique_ptr<Backend> in_audio_backend_;

    Ticker::Statistics out_stats_;
    
    std::atomic_flag tempo_imported_flag_;
    std::atomic_flag target_tempo_imported_flag_;
    std::atomic_flag accel_imported_flag_;
    std::atomic_flag meter_imported_flag_;
    std::atomic_flag sound_strong_imported_flag_;
    std::atomic_flag sound_mid_imported_flag_;
    std::atomic_flag sound_weak_imported_flag_;
    std::atomic_flag audio_backend_imported_flag_;

    mutable SpinLock mutex_;
    
    void importTempo();
    void importTargetTempo();
    void importAccel();
    void importMeter();
    void importSoundStrong();
    void importSoundMid();
    void importSoundWeak();
    void importAudioBackend();

    void importSettings();

    void exportStatistics();
    
    void startAudioBackend();
    void writeAudioBackend(const void* data, size_t bytes);
    void stopAudioBackend();

    std::unique_ptr<std::thread> audio_thread_;
    
    void startAudioThread();
    void stopAudioThread();
    void audioThreadFunction() noexcept;
  };
  
}//namespace audio
#endif//GMetronome_Ticker_h
