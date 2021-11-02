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

#include "Synthesizer.h"
#include <cassert>
#include <cmath>

namespace audio {

  Buffer generateSound(double frequency,
                       double volume,
                       SampleSpec spec,
                       microseconds duration)
  {
    assert(spec.channels == 1);
    assert(spec.format == SampleFormat::S16LE);
    
    volume = std::min( std::max(volume, 0.), 1.0);
    
    using namespace std::chrono_literals;
    
    Buffer buffer(duration, spec);

    if (volume > 0)
    {
      double sine_fac = 2 * M_PI * frequency / spec.rate;
      auto n_frames = buffer.frames();
      double one_over_n_frames = 1. / n_frames;
      double volume_drop_exp = 2. / volume;
      auto frameSize = audio::frameSize(spec);
      
      for(size_t frame = 0, buffer_index = 0;
          frame < n_frames;
          ++frame, buffer_index+=frameSize)
      {
        double v = volume * std::pow( 1. - one_over_n_frames * frame, volume_drop_exp);
        double amplitude = v * std::sin(sine_fac * frame);
        int16_t pcm =  amplitude * std::numeric_limits<int16_t>::max();
	
        buffer[buffer_index] = pcm & 0xff;
        buffer[buffer_index+1] = (pcm>>8) & 0xff;	
      }
    }
    return buffer;
  }
  
}//namespace audio
