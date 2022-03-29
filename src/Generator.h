/*
 * Copyright (C) 2021, 2022 The GMetronome Team
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
#include "Synthesizer.h"
#include "Meter.h"
#include <memory>
#include <array>

namespace audio {

  class Generator {

  public:
    struct Statistics
    {
      double  current_tempo;
      double  current_accel;
      double  current_beat;
      int     next_accent;
      microseconds  next_accent_delay;
    };

  public:
    Generator(StreamSpec spec = kDefaultSpec,
              microseconds maxChunkDuration = 80ms,
              microseconds avgChunkDuration = 50ms);

    ~Generator();

    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void swapMeter(Meter& meter);

    void updateSound(Accent accent, const SoundParameters& params);

    const Generator::Statistics& getStatistics() const;

    void start(const void*& data, size_t& bytes);
    void stop(const void*& data, size_t& bytes);
    void cycle(const void*& data, size_t& bytes);

  private:
    const StreamSpec kStreamSpec_;
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
    double accel_saved_;
    Meter meter_;
    const ByteBuffer sound_zero_;
    SoundLibrary<Accent> sounds_;
    int current_beat_;
    unsigned next_accent_;
    int frames_total_;
    int frames_done_;
    Generator::Statistics stats_;

    double convertTempoToFrameTime(double tempo);
    double convertAccelToFrameTime(double accel);

    void recalculateAccelSign();
    void recalculateFramesTotal();
    void recalculateMotionParameters();

    std::pair<double, double>
    motionAfterNFrames(double tempo, double target_tempo, double accel, size_t n_frames);

    size_t framesPerPulse(double tempo, double target_tempo,  double accel, unsigned subdiv=1);

    void updateStatistics();
  };

}//namespace audio
#endif//GMetronome_Generator_h
