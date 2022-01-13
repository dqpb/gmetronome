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
  const Glib::ustring kSchemaId                   {"org.gmetronome"};
  const Glib::ustring kSchemaIdPrefsBasename      {"preferences"};
  const Glib::ustring kSchemaIdStateBasename      {"state"};
  const Glib::ustring kSchemaIdShortcutsBasename  {"shortcuts"};

  const Glib::ustring kSchemaIdPrefs {
    kSchemaId + "." + kSchemaIdPrefsBasename
  };
  const Glib::ustring kSchemaIdState {
    kSchemaId + "." + kSchemaIdStateBasename
  };
  const Glib::ustring kSchemaIdShortcuts {
    kSchemaIdPrefs + "." + kSchemaIdShortcutsBasename
  };

  /*
   * GSettings schema paths
   */
  const Glib::ustring kSchemaPath                   {"/org/gmetronome/"};
  const Glib::ustring kSchemaPathPrefsBasename      {"preferences"};
  const Glib::ustring kSchemaPathStateBasename      {"state"};
  const Glib::ustring kSchemaPathShortcutsBasename  {"shortcuts"};

  const Glib::ustring kSchemaPathPrefs {
    kSchemaPath + kSchemaPathPrefsBasename + "/"
  };
  const Glib::ustring kSchemaPathState {
    kSchemaPath + kSchemaPathStateBasename + "/"
  };
  const Glib::ustring kSchemaPathShortcuts {
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

  /*
   * org.gmetronome.preferences keys
   */
  const Glib::ustring  kKeyPrefsVolume                    {"volume"};
  const Glib::ustring  kKeyPrefsRestoreProfile            {"restore-profile"};
  const Glib::ustring  kKeyPrefsPendulumAction            {"pendulum-action"};
  const Glib::ustring  kKeyPrefsPendulumPhaseMode         {"pendulum-phase-mode"};
  const Glib::ustring  kKeyPrefsMeterAnimation            {"meter-animation"};
  const Glib::ustring  kKeyPrefsAnimationSync             {"animation-sync"};
  const Glib::ustring  kKeyPrefsSoundStrongFrequency      {"sound-strong-frequency"};
  const Glib::ustring  kKeyPrefsSoundStrongVolume         {"sound-strong-volume"};
  const Glib::ustring  kKeyPrefsSoundStrongBalance        {"sound-strong-balance"};
  const Glib::ustring  kKeyPrefsSoundMidFrequency         {"sound-mid-frequency"};
  const Glib::ustring  kKeyPrefsSoundMidVolume            {"sound-mid-volume"};
  const Glib::ustring  kKeyPrefsSoundMidBalance           {"sound-mid-balance"};
  const Glib::ustring  kKeyPrefsSoundWeakFrequency        {"sound-weak-frequency"};
  const Glib::ustring  kKeyPrefsSoundWeakVolume           {"sound-weak-volume"};
  const Glib::ustring  kKeyPrefsSoundWeakBalance          {"sound-weak-balance"};
  const Glib::ustring  kKeyPrefsAudioBackend              {"audio-backend"};

#if HAVE_ALSA
  const Glib::ustring  kKeyPrefsAudioDeviceAlsa           {"audio-device-alsa"};
#endif
#if HAVE_OSS
  const Glib::ustring  kKeyPrefsAudioDeviceOss            {"audio-device-oss"};
#endif
#if HAVE_PULSEAUDIO
  const Glib::ustring  kKeyPrefsAudioDevicePulseaudio     {"audio-device-pulseaudio"};
#endif

  // map audio backend identifier (w/o kAudioBackendNone) to the corresponding
  // audio device settings key (e.g. kAudioBackendAlsa -> "audio-device-alsa")
  // and vice versa
  extern const std::map<settings::AudioBackend, Glib::ustring> kBackendToDeviceMap;
  extern const std::map<Glib::ustring, settings::AudioBackend> kDeviceToBackendMap;

  /*
   * org.gmetronome.preferences.shortcuts keys
   */
  const Glib::ustring  kKeyShortcutsQuit                  {"quit"};
  const Glib::ustring  kKeyShortcutsShowPrimaryMenu       {"show-primary-menu"};
  const Glib::ustring  kKeyShortcutsShowProfiles          {"show-profiles"};
  const Glib::ustring  kKeyShortcutsShowPreferences       {"show-preferences"};
  const Glib::ustring  kKeyShortcutsShowShortcuts         {"show-shortcuts"};
  const Glib::ustring  kKeyShortcutsShowAbout             {"show-about"};
  const Glib::ustring  kKeyShortcutsShowHelp              {"show-help"};
  const Glib::ustring  kKeyShortcutsShowPendulum          {"show-pendulum"};
  const Glib::ustring  kKeyShortcutsShowMeter             {"show-meter"};
  const Glib::ustring  kKeyShortcutsShowTrainer           {"show-trainer"};
  const Glib::ustring  kKeyShortcutsFullScreen            {"full-screen"};
  const Glib::ustring  kKeyShortcutsStart                 {"start"};
  const Glib::ustring  kKeyShortcutsVolumeIncrease1       {"volume-increase-1"};
  const Glib::ustring  kKeyShortcutsVolumeDecrease1       {"volume-decrease-1"};
  const Glib::ustring  kKeyShortcutsVolumeIncrease10      {"volume-increase-10"};
  const Glib::ustring  kKeyShortcutsVolumeDecrease10      {"volume-decrease-10"};
  const Glib::ustring  kKeyShortcutsTempoIncrease1        {"tempo-increase-1"};
  const Glib::ustring  kKeyShortcutsTempoDecrease1        {"tempo-decrease-1"};
  const Glib::ustring  kKeyShortcutsTempoIncrease10       {"tempo-increase-10"};
  const Glib::ustring  kKeyShortcutsTempoDecrease10       {"tempo-decrease-10"};
  const Glib::ustring  kKeyShortcutsTempoTap              {"tempo-tap"};
  const Glib::ustring  kKeyShortcutsMeterEnabled          {"meter-enabled"};
  const Glib::ustring  kKeyShortcutsMeterSelect1Simple    {"meter-select-1-simple"};
  const Glib::ustring  kKeyShortcutsMeterSelect2Simple    {"meter-select-2-simple"};
  const Glib::ustring  kKeyShortcutsMeterSelect3Simple    {"meter-select-3-simple"};
  const Glib::ustring  kKeyShortcutsMeterSelect4Simple    {"meter-select-4-simple"};
  const Glib::ustring  kKeyShortcutsMeterSelect1Compound  {"meter-select-1-compound"};
  const Glib::ustring  kKeyShortcutsMeterSelect2Compound  {"meter-select-2-compound"};
  const Glib::ustring  kKeyShortcutsMeterSelect3Compound  {"meter-select-3-compound"};
  const Glib::ustring  kKeyShortcutsMeterSelect4Compound  {"meter-select-4-compound"};
  const Glib::ustring  kKeyShortcutsMeterSelectCustom     {"meter-select-custom"};
  const Glib::ustring  kKeyShortcutsTrainerEnabled        {"trainer-enabled"};

  /*
   * org.gmetronome.state keys
   */
  const Glib::ustring  kKeyStateProfilesSelect            {"profiles-select"};
  const Glib::ustring  kKeyStateShowMeter                 {"show-meter"};
  const Glib::ustring  kKeyStateShowTrainer               {"show-trainer"};
  const Glib::ustring  kKeyStateShowPendulum              {"show-pendulum"};

}//namespace settings
#endif//GMetronome_Settings_h
