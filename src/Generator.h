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

#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "AudioBuffer.h"
#include "Meter.h"

namespace audio {

  struct Statistics
  {
    double    current_tempo;
    double    current_accel;
    int       next_accent;
    uint64_t  next_accent_time;
    uint64_t  latency;
  };
  
  class Generator : public AbstractAudioSource {
    
  public:    

    Generator(SampleSpec spec = { SampleFormat::S16LE, 44100, 1 },
              microseconds maxChunkDuration = 80ms,
              microseconds avgChunkDuration = 50ms);
    
    ~Generator();
    
    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void setMeter(const Meter& meter);
    void setMeter(Meter&& meter);
    
    void setSoundStrong(Buffer&& sound);
    void setSoundMid(Buffer&& sound);
    void setSoundWeak(Buffer&& sound);

    Statistics getStatistics() const;
    
    std::unique_ptr<AbstractAudioSink>
    swapSink(std::unique_ptr<AbstractAudioSink> sink) override;
    
    void start() override;
    void stop() override;
    void cycle() override;
    
  private:
    const SampleSpec kSampleSpec_;
    const microseconds kMaxChunkDuration_;
    const microseconds kAvgChunkDuration_;
    const size_t kMaxChunkFrames_;
    const size_t kAvgChunkFrames_;
    const double kMinutesFramesRatio_;
    const double kMicrosecondsFramesRatio_;
    const double kFramesMinutesRatio_;
    const double kFramesMicrosecondsRatio_;

    double tempo_;
    double target_tempo_;
    double accel_;
    Meter meter_;
    const Buffer sound_zero_;
    Buffer sound_weak_;
    Buffer sound_mid_;
    Buffer sound_strong_;
    std::unique_ptr<AbstractAudioSink> audio_sink_;

    std::atomic<double> in_tempo_;
    std::atomic<double> in_target_tempo_;
    std::atomic<double> in_accel_;
    Meter in_meter_;
    Buffer in_sound_weak_;
    Buffer in_sound_mid_;
    Buffer in_sound_strong_;    
    std::unique_ptr<AbstractAudioSink> in_audio_sink_;
    
    std::atomic<double>   out_current_tempo_;
    std::atomic<double>   out_current_accel_;
    std::atomic<uint64_t> out_next_accent_;
    std::atomic<uint64_t> out_latency_;
    
    std::mutex mutex_;    
    std::condition_variable condition_var_;
    bool condition_var_flag_;
    
    std::atomic_flag tempo_import_flag_;
    std::atomic_flag accel_import_flag_;
    std::atomic_flag target_tempo_import_flag_;
    std::atomic_flag meter_import_flag_;
    std::atomic_flag sound_weak_import_flag_;
    std::atomic_flag sound_mid_import_flag_;
    std::atomic_flag sound_strong_import_flag_;
    std::atomic_flag audio_sink_import_flag_;
            
    unsigned next_accent_;
    
    size_t frames_done_;
    size_t frames_left_;
    size_t frames_chunk_;
    size_t frames_total_;

    uint64_t latency_;

    void importTempo();
    void importTargetTempo();
    void importAccel();
    void importMeter();
    void importSoundStrong();
    void importSoundMid();
    void importSoundWeak();
    void importAudioSink();
    
    double convertTempoToFrameTime(double tempo);
    double convertAccelToFrameTime(double accel);
    
    void recalculateAccelSign();
    void recalculateFramesTotal();
    void recalculateTempo();
    void recalculateLatency();
    
    double tempoAfterNFrames(double tempo, double target_tempo, double accel, size_t n_frames);
    double accelAfterNFrames(double tempo, double target_tempo, double accel, size_t n_frames);
    size_t framesPerPulse(double tempo, double target_tempo,  double accel, unsigned subdiv=1);

    void exportCurrentTempo();
    void exportCurrentAccel();
    void exportNextAccent();
    void exportLatency();
  };
  
  Buffer generateSound(double frequency,
                       double volume,
                       SampleSpec spec,
                       microseconds duration);
}//namespace
#endif//__GENERATOR_H__
