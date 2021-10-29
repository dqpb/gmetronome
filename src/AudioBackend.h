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

#ifndef GMetronome_AudioBackend_h
#define GMetronome_AudioBackend_h

#include "Audio.h"
#include "Settings.h"
#include <vector>

namespace audio {
  
  class AbstractAudioSink {
  public:
    virtual ~AbstractAudioSink() {}
    virtual void start() {}
    virtual void stop() {}
    virtual void write(const void* data, size_t bytes) = 0;
    virtual void flush() = 0;
    virtual void drain() = 0;
    virtual uint64_t latency() { return 0; }
  };
  
  /**
   *  Returns a list of available audio backends, that can be instantiated
   *  with createBackend().
   */
  const std::vector<AudioBackend>& availableBackends();
  
  /** Create a new audio backend. */
  std::unique_ptr<AbstractAudioSink> createBackend(AudioBackend backend);
  
}//namespace audio
#endif//GMetronome_AudioBackend_h
