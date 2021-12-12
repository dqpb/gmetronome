/*
 * Copyright (C) 2020, 2021 The GMetronome Team
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

#include "Settings.h"

namespace settings {
  
  const std::map<settings::AudioBackend, Glib::ustring> kBackendToDeviceMap
  {
#if HAVE_ALSA
    { settings::kAudioBackendAlsa, settings::kKeyPrefsAudioDeviceAlsa },
#endif
#if HAVE_OSS
    { settings::kAudioBackendOss, settings::kKeyPrefsAudioDeviceOss },
#endif
#if HAVE_PULSEAUDIO
    { settings::kAudioBackendPulseaudio, settings::kKeyPrefsAudioDevicePulseaudio },
#endif
  };

  const std::map<Glib::ustring, settings::AudioBackend> kDeviceToBackendMap
  {
#if HAVE_ALSA
    { settings::kKeyPrefsAudioDeviceAlsa, settings::kAudioBackendAlsa },
#endif
#if HAVE_OSS
    { settings::kKeyPrefsAudioDeviceOss, settings::kAudioBackendOss },
#endif
#if HAVE_PULSEAUDIO
    { settings::kKeyPrefsAudioDevicePulseaudio, settings::kAudioBackendPulseaudio },
#endif
  };

}//namespace settings
