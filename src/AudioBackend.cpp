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
#include "AudioBackend.h"
#include "AudioBackendDummy.h"

#ifdef HAVE_ALSA
#include "Alsa.h"
#endif
#ifdef HAVE_OSS
#include "Oss.h"
#endif
#ifdef HAVE_PULSEAUDIO
#include "PulseAudio.h"
#endif

namespace audio {
  
  const std::vector<settings::AudioBackend>& availableBackends()
  {
    static const std::vector<settings::AudioBackend> backends = {
      settings::kAudioBackendNone,
#ifdef HAVE_ALSA
      settings::kAudioBackendAlsa,
#endif
#ifdef HAVE_OSS
      settings::kAudioBackendOss,
#endif
#ifdef HAVE_PULSEAUDIO
      settings::kAudioBackendPulseaudio,
#endif
    };
    return backends;
  }
  
  std::unique_ptr<Backend> createBackend(settings::AudioBackend id)
  {
    std::unique_ptr<Backend> backend = nullptr;
    
    switch (id)
    {
#ifdef HAVE_ALSA
    case settings::kAudioBackendAlsa:
      backend = std::make_unique<AlsaBackend>();
      break;
#endif
#ifdef HAVE_OSS
    case settings::kAudioBackendOss:
      backend = std::make_unique<OssBackend>();
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case settings::kAudioBackendPulseaudio:
      backend = std::make_unique<PulseAudioBackend>();
      break;
#endif
    case settings::kAudioBackendNone:
      backend = std::make_unique<DummyBackend>();
      break;
      
    default:
      return nullptr;
    };

    return backend;
  }
  
}//namespace audio
