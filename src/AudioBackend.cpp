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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

  const std::vector<BackendIdentifier>& availableBackends()
  {
    static const std::vector<BackendIdentifier> backends = {
      BackendIdentifier::kNone,
#ifdef HAVE_ALSA
      BackendIdentifier::kALSA,
#endif
#ifdef HAVE_OSS
      BackendIdentifier::kOSS,
#endif
#ifdef HAVE_PULSEAUDIO
      BackendIdentifier::kPulseAudio,
#endif
    };
    return backends;
  }

  std::unique_ptr<Backend> createBackend(BackendIdentifier id)
  {
    std::unique_ptr<Backend> backend = nullptr;

    switch (id)
    {
#ifdef HAVE_ALSA
    case BackendIdentifier::kALSA:
      backend = std::make_unique<AlsaBackend>();
      break;
#endif
#ifdef HAVE_OSS
    case BackendIdentifier::kOSS:
      backend = std::make_unique<OssBackend>();
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case BackendIdentifier::kPulseAudio:
      backend = std::make_unique<PulseAudioBackend>();
      break;
#endif
    case BackendIdentifier::kNone:
      backend = std::make_unique<DummyBackend>();
      break;

    default:
      return nullptr;
    };

    return backend;
  }

}//namespace audio
