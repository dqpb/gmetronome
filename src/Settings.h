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

#ifndef GMetronome_Settings_h
#define GMetronome_Settings_h

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <glibmm/ustring.h>
#include <map>

namespace settings {

  /*
   * GSettings schema id's
   */
  inline const Glib::ustring kSchemaId                   {"org.gmetronome"};
  inline const Glib::ustring kSchemaIdPrefsBasename      {"preferences"};
  inline const Glib::ustring kSchemaIdStateBasename      {"state"};
  inline const Glib::ustring kSchemaIdShortcutsBasename  {"shortcuts"};

  inline const Glib::ustring kSchemaIdPrefs {
    kSchemaId + "." + kSchemaIdPrefsBasename
  };
  inline const Glib::ustring kSchemaIdState {
    kSchemaId + "." + kSchemaIdStateBasename
  };
  inline const Glib::ustring kSchemaIdShortcuts {
    kSchemaIdPrefs + "." + kSchemaIdShortcutsBasename
  };

  /*
   * GSettings schema paths
   */
  inline const Glib::ustring kSchemaPath                   {"/org/gmetronome/"};
  inline const Glib::ustring kSchemaPathPrefsBasename      {"preferences"};
  inline const Glib::ustring kSchemaPathStateBasename      {"state"};
  inline const Glib::ustring kSchemaPathShortcutsBasename  {"shortcuts"};

  inline const Glib::ustring kSchemaPathPrefs {
    kSchemaPath + kSchemaPathPrefsBasename + "/"
  };
  inline const Glib::ustring kSchemaPathState {
    kSchemaPath + kSchemaPathStateBasename + "/"
  };
  inline const Glib::ustring kSchemaPathShortcuts {
    kSchemaPathPrefs + kSchemaPathShortcutsBasename + "/"
  };

  /*
   * org.gmetronome enum types
   */
  enum AudioBackend
  {
    kAudioBackendNone       = 0,
#if HAVE_ALSA
    kAudioBackendAlsa       = 1,
#endif
#if HAVE_OSS
    kAudioBackendOss        = 2,
#endif
#if HAVE_PULSEAUDIO
    kAudioBackendPulseaudio = 3,
#endif
  };

  enum PendulumAction
  {
    kPendulumActionCenter = 0,
    kPendulumActionReal   = 1,
    kPendulumActionEdge   = 2,
  };

  enum PendulumPhaseMode
  {
    kPendulumPhaseModeLeft = 0,
    kPendulumPhaseModeRight = 1
  };

  inline constexpr  double    kDefaultVolume    = 80;
  inline constexpr  double    kMinimumVolume    = 0;
  inline constexpr  double    kMaximumVolume    = 100;

  /*
   * org.gmetronome.preferences keys
   */
  inline const Glib::ustring  kKeyPrefsVolume                    {"volume"};
  inline const Glib::ustring  kKeyPrefsRestoreProfile            {"restore-profile"};
  inline const Glib::ustring  kKeyPrefsPendulumAction            {"pendulum-action"};
  inline const Glib::ustring  kKeyPrefsPendulumPhaseMode         {"pendulum-phase-mode"};
  inline const Glib::ustring  kKeyPrefsMeterAnimation            {"meter-animation"};
  inline const Glib::ustring  kKeyPrefsAnimationSync             {"animation-sync"};
  inline const Glib::ustring  kKeyPrefsSoundStrongFrequency      {"sound-strong-frequency"};
  inline const Glib::ustring  kKeyPrefsSoundStrongVolume         {"sound-strong-volume"};
  inline const Glib::ustring  kKeyPrefsSoundStrongBalance        {"sound-strong-balance"};
  inline const Glib::ustring  kKeyPrefsSoundMidFrequency         {"sound-mid-frequency"};
  inline const Glib::ustring  kKeyPrefsSoundMidVolume            {"sound-mid-volume"};
  inline const Glib::ustring  kKeyPrefsSoundMidBalance           {"sound-mid-balance"};
  inline const Glib::ustring  kKeyPrefsSoundWeakFrequency        {"sound-weak-frequency"};
  inline const Glib::ustring  kKeyPrefsSoundWeakVolume           {"sound-weak-volume"};
  inline const Glib::ustring  kKeyPrefsSoundWeakBalance          {"sound-weak-balance"};
  inline const Glib::ustring  kKeyPrefsAudioBackend              {"audio-backend"};

#if HAVE_ALSA
  inline const Glib::ustring  kKeyPrefsAudioDeviceAlsa           {"audio-device-alsa"};
#endif
#if HAVE_OSS
  inline const Glib::ustring  kKeyPrefsAudioDeviceOss            {"audio-device-oss"};
#endif
#if HAVE_PULSEAUDIO
  inline const Glib::ustring  kKeyPrefsAudioDevicePulseaudio     {"audio-device-pulseaudio"};
#endif

  // map audio backend identifier (w/o kAudioBackendNone) to the corresponding
  // audio device settings key (e.g. kAudioBackendAlsa -> "audio-device-alsa")
  // and vice versa
  extern const std::map<settings::AudioBackend, Glib::ustring> kBackendToDeviceMap;
  extern const std::map<Glib::ustring, settings::AudioBackend> kDeviceToBackendMap;

  /*
   * org.gmetronome.preferences.shortcuts keys
   */
  inline const Glib::ustring  kKeyShortcutsQuit                  {"quit"};
  inline const Glib::ustring  kKeyShortcutsShowPrimaryMenu       {"show-primary-menu"};
  inline const Glib::ustring  kKeyShortcutsShowProfiles          {"show-profiles"};
  inline const Glib::ustring  kKeyShortcutsShowPreferences       {"show-preferences"};
  inline const Glib::ustring  kKeyShortcutsShowShortcuts         {"show-shortcuts"};
  inline const Glib::ustring  kKeyShortcutsShowAbout             {"show-about"};
  inline const Glib::ustring  kKeyShortcutsShowHelp              {"show-help"};
  inline const Glib::ustring  kKeyShortcutsShowPendulum          {"show-pendulum"};
  inline const Glib::ustring  kKeyShortcutsFullScreen            {"full-screen"};
  inline const Glib::ustring  kKeyShortcutsStart                 {"start"};
  inline const Glib::ustring  kKeyShortcutsVolumeIncrease1       {"volume-increase-1"};
  inline const Glib::ustring  kKeyShortcutsVolumeDecrease1       {"volume-decrease-1"};
  inline const Glib::ustring  kKeyShortcutsVolumeIncrease10      {"volume-increase-10"};
  inline const Glib::ustring  kKeyShortcutsVolumeDecrease10      {"volume-decrease-10"};
  inline const Glib::ustring  kKeyShortcutsTempoIncrease1        {"tempo-increase-1"};
  inline const Glib::ustring  kKeyShortcutsTempoDecrease1        {"tempo-decrease-1"};
  inline const Glib::ustring  kKeyShortcutsTempoIncrease10       {"tempo-increase-10"};
  inline const Glib::ustring  kKeyShortcutsTempoDecrease10       {"tempo-decrease-10"};
  inline const Glib::ustring  kKeyShortcutsTempoTap              {"tempo-tap"};
  inline const Glib::ustring  kKeyShortcutsMeterEnabled          {"meter-enabled"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple2    {"meter-select-simple-2"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple3    {"meter-select-simple-3"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple4    {"meter-select-simple-4"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound2  {"meter-select-compound-2"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound3  {"meter-select-compound-3"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound4  {"meter-select-compound-4"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCustom     {"meter-select-custom"};
  inline const Glib::ustring  kKeyShortcutsTrainerEnabled        {"trainer-enabled"};

  /*
   * org.gmetronome.state keys
   */
  inline const Glib::ustring  kKeyStateProfilesSelect            {"profiles-select"};
  inline const Glib::ustring  kKeyStateShowPendulum              {"show-pendulum"};

}//namespace settings
#endif//GMetronome_Settings_h
