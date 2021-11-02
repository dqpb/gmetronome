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

#include "Ticker.h"
#include "Synthesizer.h"
#include "PulseAudio.h"

#include <iostream>

namespace audio {

  namespace
  {
    using namespace std::chrono_literals;
    
    static constexpr SampleSpec    kSampleSpec       = { SampleFormat::S16LE, 44100, 1 };
    static constexpr microseconds  kMaxChunkDuration = 80ms;
    static constexpr microseconds  kAvgChunkDuration = 50ms;
    static constexpr double        kSineVolume       = 1.;
    static constexpr microseconds  kSineDuration     = 60ms;
  }
  
  // Ticker
  Ticker::Ticker()
    : generator_(kSampleSpec, kMaxChunkDuration, kAvgChunkDuration),
      tempo_range_({30,250}),
      state_(TickerState::kReady),
      except_(nullptr)
  {}
  
  Ticker::~Ticker()
  {
    stop();
  }  

  void Ticker::setTempo(double tempo)
  { generator_.setTempo(clamp(tempo, tempo_range_)); }

  void Ticker::setTargetTempo(double target_tempo)
  { generator_.setTargetTempo(clamp(target_tempo, tempo_range_)); }

  void Ticker::setAccel(double accel)
  { generator_.setAccel(accel);}

  void Ticker::setMeter(const Meter& meter)
  { generator_.setMeter(meter); }
  
  void Ticker::setMeter(Meter&& meter)
  { generator_.setMeter(std::move(meter));}

  void Ticker::setSoundStrong(double frequency, double volume)
  {
    generator_.setSoundStrong(
      generateSound(frequency, volume, kSampleSpec, kSineDuration));
  }
  
  void Ticker::setSoundMid(double frequency, double volume)
  {
    generator_.setSoundMid(
      generateSound(frequency, volume, kSampleSpec, kSineDuration));
  }
  
  void Ticker::setSoundWeak(double frequency, double volume)
  {
    generator_.setSoundWeak(
      generateSound(frequency, volume, kSampleSpec, kSineDuration));
  }

  void Ticker::setTempoRange(Range<double> range)
  {
    if (range[kRangeMinIndex]>range[kRangeMaxIndex]) {
      //TODO: handle error
      return;
    }
    // TODO: clamp tempo_, target_tempo_ and start_tempo_
    tempo_range_.swap(range);
  }

  Statistics Ticker::getStatistics() const
  {
    return generator_.getStatistics();
  }
  
  const Range<double>& Ticker::getTempoRange() const
  {
    return tempo_range_;
  }

  void Ticker::setAccelRange(Range<double> range)
  {
    if (range[kRangeMinIndex]>range[kRangeMaxIndex]) {
      //TODO: handle error
      return;
    }
    // TODO: clamp accel_
    accel_range_.swap(range);
  }
  
  const Range<double>& Ticker::getAccelRange() const
  {
    return accel_range_;
  }
  
  void Ticker::start()
  {
    TickerState state = state_.load();

    if (state == TickerState::kPlaying) 
      return;
    if (state == TickerState::kError)
      return;
    
    state_ = TickerState::kPlaying;
    startWorker();
  }
    
  void Ticker::stop()
  {
    TickerState state = state_.load();
    if (state == TickerState::kError)
      return;
    
    stopWorker();
  }
    
  TickerState Ticker::state() const noexcept
  {
    return state_.load();
  }

  void Ticker::reset()
  {
    if (except_ != nullptr) {
      try {
	std::rethrow_exception(except_);
      }
      catch (const PulseAudioError& e) {
	// handle error
      }
      catch (...) {
	// handle error
      }
    }
    stopWorker();
  }
    
  void Ticker::startWorker() noexcept
  {
    if (worker_thread_ != nullptr)
      return;
    
    try {
      
      worker_thread_
        = std::make_unique<std::thread>(&Ticker::worker, this);

    } catch(...) {}
  }
  
  void Ticker::stopWorker() noexcept
  {
    if (!worker_thread_)
      return;
    
    state_ = TickerState::kReady;
    try { worker_thread_->join(); } catch(...) {}
    worker_thread_.reset();  
    except_ = nullptr;
  }
  
  void Ticker::worker() noexcept
  {
    try {
      std::unique_ptr<AbstractAudioSink> pa_sink
        = std::make_unique<PulseAudioConnection>(kSampleSpec); 
      
      generator_.swapSink(std::move(pa_sink));      
      generator_.start();
      
      while ( state_.load() != TickerState::kReady )
      {
        generator_.cycle();
      }
      
      generator_.stop();
      pa_sink = generator_.swapSink(nullptr); 
      
      pa_sink->drain();
    }
    catch(...)
    {
      except_ = std::current_exception();
      state_ = TickerState::kError;
    }
  }  
    
}//namespace audio
