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

#include <mutex>
#include <memory>
#include <thread>
#include <atomic>

#include "Generator.h"
#include "AudioBuffer.h"
#include "Meter.h"

namespace audio {

  class Ticker;
  
  enum class TickerState {
    kReady,
    kPlaying,
    kError
  };
  
  template<class T>
  using Range = std::array<T,2>;
  const std::size_t kRangeMinIndex = 0;
  const std::size_t kRangeMaxIndex = 1;
  
  class Ticker {
  public:
    Ticker();
    ~Ticker();
    
    void start();
    void stop();
    void reset();
    
    TickerState state() const noexcept;

    void setTempo(double tempo);
    
    void setTargetTempo(double target_tempo);
    
    void setAccel(double accel);
    
    void setMeter(const Meter& meter);
    void setMeter(Meter&& meter);
    
    void setSoundStrong(double frequency, double volume);
    void setSoundMid(double frequency, double volume);
    void setSoundWeak(double frequency, double volume);
    
    audio::Statistics getStatistics() const;
    
    void setTempoRange(Range<double> range);
    const Range<double>& getTempoRange() const;
    
    void setAccelRange(Range<double> range);
    const Range<double>& getAccelRange() const;

  private:
    Generator generator_;
    
    Range<double> tempo_range_;
    Range<double> accel_range_;
    
    std::atomic<TickerState> state_;
    
    std::exception_ptr except_;
    mutable std::recursive_mutex mutex_;    
    std::unique_ptr<std::thread> worker_thread_;
    
    void startWorker() noexcept;
    void stopWorker() noexcept;
    void worker() noexcept;
  };
  
  /** Clamp values to the inclusive range of min and max. */
  template<class T>
  T clamp(T val, const Range<T>& range)
  {
    return std::min( std::max(val, range[kRangeMinIndex]), range[kRangeMaxIndex] );
  }

}//namespace audio
#endif//GMetronome_Ticker_h
