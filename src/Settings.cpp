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
#include "Meter.h"
#include "AudioBackend.h"
#include "Error.h"

namespace settings {

  const std::map<Accent, Glib::ustring> kSchemaPathSoundThemeParamsBasenameMap =
  {
    { kAccentOff, "" },
    { kAccentWeak, kSchemaPathSoundThemeWeakParamsBasename },
    { kAccentMid, kSchemaPathSoundThemeMidParamsBasename },
    { kAccentStrong, kSchemaPathSoundThemeStrongParamsBasename }
  };

  const std::map<settings::AudioBackend, audio::BackendIdentifier> kAudioBackendToIdentifierMap
  {
    {settings::kAudioBackendNone, audio::BackendIdentifier::kNone},
#if HAVE_ALSA
    {settings::kAudioBackendAlsa, audio::BackendIdentifier::kALSA},
#endif
#if HAVE_OSS
    {settings::kAudioBackendOss, audio::BackendIdentifier::kOSS},
#endif
#if HAVE_PULSEAUDIO
    {settings::kAudioBackendPulseaudio, audio::BackendIdentifier::kPulseAudio},
#endif
  };

  AudioBackend audioBackendFromIdentifier(audio::BackendIdentifier id)
  {
    auto it = std::find_if(kAudioBackendToIdentifierMap.begin(),
                           kAudioBackendToIdentifierMap.end(),
                           [&] (const auto& e) { return e.second == id; });

    if (it == kAudioBackendToIdentifierMap.end())
      throw GMetronomeError {"invalid audio backend identifier"};

    return it->first;
  }

  audio::BackendIdentifier audioBackendToIdentifier(AudioBackend backend)
  { return kAudioBackendToIdentifierMap.at(backend); }


  std::vector<AudioBackend> availableBackends()
  {
    const auto& ids = audio::availableBackends();
    std::vector<settings::AudioBackend> backends;

    std::transform(ids.begin(), ids.end(), std::back_inserter(backends),
                   [] (auto& id) { return settings::audioBackendFromIdentifier(id); });

    return backends;
  }

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

  Glib::RefPtr<Gio::Settings> settings()
  {
    static Glib::RefPtr<Gio::Settings> s = Gio::Settings::create(kSchemaId);;
    return s;
  }

  Glib::RefPtr<Gio::Settings> preferences()
  {
    static Glib::RefPtr<Gio::Settings> s = settings()->get_child(kSchemaPathPrefsBasename);
    return s;
  }

  Glib::RefPtr<Gio::Settings> sound()
  {
    static Glib::RefPtr<Gio::Settings> s = preferences()->get_child(kSchemaPathSoundBasename);
    return s;
  }

  Glib::RefPtr<SettingsList<SoundTheme>> soundThemes()
  {
    static Glib::RefPtr<SettingsList<SoundTheme>> sl = SettingsList<SoundTheme>::create(
      sound()->get_child(kSchemaPathSoundThemesBasename), kSchemaIdSoundTheme);
    return sl;
  }

  Glib::RefPtr<Gio::Settings> shortcuts()
  {
    static Glib::RefPtr<Gio::Settings> s = preferences()->get_child(kSchemaPathShortcutsBasename);
    return s;
  }

  Glib::RefPtr<Gio::Settings> state()
  {
    static Glib::RefPtr<Gio::Settings> s = settings()->get_child(kSchemaPathStateBasename);
    return s;
  }

}//namespace settings
