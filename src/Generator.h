/*
 * Copyright (C) 2021 The GMetronome Team
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

#ifndef GMetronome_Generator_h
#define GMetronome_Generator_h

#include "AudioBuffer.h"
#include "Meter.h"
#include <memory>

namespace audio {
  
  class Generator {

  public:

    struct Statistics
    {
      double  current_tempo;
      double  current_accel;
      int     next_accent;
      microseconds  next_accent_time;
    };

  public:    

    Generator(SampleSpec spec = { SampleFormat::S16LE, 44100, 1 },
              microseconds maxChunkDuration = 80ms,
              microseconds avgChunkDuration = 50ms);
    
    ~Generator();
    
    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void swapMeter(Meter& meter);
    
    void setSoundStrong(Buffer& sound);
    void setSoundMid(Buffer& sound);
    void setSoundWeak(Buffer& sound);
    
    const Generator::Statistics& getStatistics() const;
    
    void start(const void*& data, size_t& bytes);
    void stop(const void*& data, size_t& bytes);
    void cycle(const void*& data, size_t& bytes);
    
  private:
    const SampleSpec kSampleSpec_;
    const microseconds kMaxChunkDuration_;
    const microseconds kAvgChunkDuration_;
    const int kMaxChunkFrames_;
    const int kAvgChunkFrames_;
    const double kMinutesFramesRatio_;
    const double kMicrosecondsFramesRatio_;
    const double kFramesMinutesRatio_;
    const double kFramesMicrosecondsRatio_;

    double tempo_;
    double target_tempo_;
    double accel_;
    Meter meter_;
    const Buffer sound_zero_;
    Buffer sound_strong_;
    Buffer sound_mid_;
    Buffer sound_weak_;
    unsigned next_accent_;
    int frames_done_;
    int frames_left_;
    int frames_total_;
    Generator::Statistics stats_;
    
    double convertTempoToFrameTime(double tempo);
    double convertAccelToFrameTime(double accel);
    
    void recalculateAccelSign();
    void recalculateFramesTotal();
    void recalculateTempo();
    
    double tempoAfterNFrames(double tempo, double target_tempo, double accel, size_t n_frames);
    double accelAfterNFrames(double tempo, double target_tempo, double accel, size_t n_frames);
    size_t framesPerPulse(double tempo, double target_tempo,  double accel, unsigned subdiv=1);
    
    void updateStatistics();
  };
  
  Buffer generateSound(double frequency,
                       double volume,
                       SampleSpec spec,
                       microseconds duration);
  
}//namespace audio
#endif//GMetronome_Generator_h
