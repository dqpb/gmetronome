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

#include "config.h"

#ifdef HAVE_ALSA 
#include "Alsa.h"
#endif
#ifdef HAVE_PULSEAUDIO
#include "PulseAudio.h"
#endif

namespace audio {
  
  const std::vector<AudioBackend> availableBackends()
  {
    static const std::vector<AudioBackend> backends = {
#ifdef HAVE_PULSEAUDIO
      kAudioBackendPulseaudio,
#endif
#ifdef HAVE_ALSA
      kAudioBackendAlsa,
#endif
    };
    return backends;
  }
  
  std::unique_ptr<AbstractAudioSink> createBackend(AudioBackend backend)
  {
    static constexpr SampleSpec kSampleSpec = { SampleFormat::S16LE, 44100, 1 };
    
    switch (backend)
    {
#ifdef HAVE_PULSEAUDIO
    case kAudioBackendPulseaudio:
      return std::make_unique<PulseAudioConnection>(kSampleSpec); 
#endif
    default:
      return nullptr;
    };
  }
  
}//namespace audio
