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

#ifndef GMetronome_AudioSource_h
#define GMetronome_AudioSource_h

#include "Audio.h"

namespace audio {

  class AbstractAudioSink; // see AudioBackend.h
  
  class AbstractAudioSource {
  public:
    virtual ~AbstractAudioSource() {}
    virtual void start() {}
    virtual void stop() {}
    virtual void cycle() = 0;
    virtual std::unique_ptr<AbstractAudioSink>
    swapSink(std::unique_ptr<AbstractAudioSink> sink) = 0;
  };

}//namespace audio
#endif//GMetronome_AudioSource_h
