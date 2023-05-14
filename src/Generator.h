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
#include "SoundLibrary.h"
#include "Meter.h"
#include <memory>
#include <array>

namespace audio {

  class Generator {

  public:
    struct Statistics
    {
      double current_tempo {0.0};
      double current_accel {0.0};
      double current_beat {0.0};
      int next_accent {0};
      microseconds next_accent_delay {0us};
    };

  public:
    Generator(const StreamSpec& spec = kDefaultSpec);

    void prepare(const StreamSpec& spec);

    void setTempo(double tempo);
    void setTargetTempo(double target_tempo);
    void setAccel(double accel);
    void swapMeter(Meter& meter);

    void setBeatPosition(double beat);

    void setSound(Accent accent, const SoundParameters& params);

    void start(const void*& data, size_t& bytes);
    void stop(const void*& data, size_t& bytes);
    void cycle(const void*& data, size_t& bytes);

    const Generator::Statistics& getStatistics() const;

  private:
    StreamSpec spec_;
    int maxChunkFrames_;
    int avgChunkFrames_;
    double tempo_;
    double target_tempo_;
    double accel_;
    double accel_saved_;
    Meter meter_;
    ByteBuffer sound_zero_;
    SoundLibrary sounds_;
    int current_beat_;
    unsigned next_accent_;
    int frames_total_;
    int frames_done_;
    Generator::Statistics stats_;

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
