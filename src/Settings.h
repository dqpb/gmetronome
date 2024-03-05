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

#include "Meter.h"

#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <giomm/settings.h>
#include <map>
#include <tuple>

namespace settings {

  /*
   * GSettings schema id's
   */
  inline const Glib::ustring kSchemaId                         {PACKAGE_ID};
  inline const Glib::ustring kSchemaIdPrefsBasename            {"preferences"};
  inline const Glib::ustring kSchemaIdStateBasename            {"state"};
  inline const Glib::ustring kSchemaIdSoundBasename            {"sound"};
  inline const Glib::ustring kSchemaIdSoundThemeBasename       {"theme"};
  inline const Glib::ustring kSchemaIdSoundThemeListBasename   {"theme-list"};
  inline const Glib::ustring kSchemaIdSoundThemeParamsBasename {"parameters"};
  inline const Glib::ustring kSchemaIdShortcutsBasename        {"shortcuts"};

  inline const Glib::ustring kSchemaIdPrefs {
    kSchemaId + "." + kSchemaIdPrefsBasename
  };
  inline const Glib::ustring kSchemaIdState {
    kSchemaId + "." + kSchemaIdStateBasename
  };
  inline const Glib::ustring kSchemaIdSound {
    kSchemaIdPrefs + "." + kSchemaIdSoundBasename
  };
  inline const Glib::ustring kSchemaIdSoundTheme {
    kSchemaIdSound + "." + kSchemaIdSoundThemeBasename
  };
  inline const Glib::ustring kSchemaIdSoundThemeList {
    kSchemaIdSound + "." + kSchemaIdSoundThemeListBasename
  };
  inline const Glib::ustring kSchemaIdShortcuts {
    kSchemaIdPrefs + "." + kSchemaIdShortcutsBasename
  };

  /*
   * GSettings schema paths
   */
  inline const Glib::ustring kSchemaPath                        {PACKAGE_ID_PATH"/"};
  inline const Glib::ustring kSchemaPathPrefsBasename           {"preferences"};
  inline const Glib::ustring kSchemaPathStateBasename           {"state"};
  inline const Glib::ustring kSchemaPathSoundBasename           {"sound"};
  inline const Glib::ustring kSchemaPathSoundThemesBasename     {"themes"};
  inline const Glib::ustring kSchemaPathShortcutsBasename       {"shortcuts"};

  inline const Glib::ustring kSchemaPathSoundThemeStrongParamsBasename {"strong-params"};
  inline const Glib::ustring kSchemaPathSoundThemeMidParamsBasename    {"mid-params"};
  inline const Glib::ustring kSchemaPathSoundThemeWeakParamsBasename   {"weak-params"};

  // map accent to sound theme parameters basename
  extern const std::map<Accent, Glib::ustring> kSchemaPathSoundThemeParamsBasenameMap;

  inline const Glib::ustring kSchemaPathPrefs {
    kSchemaPath + kSchemaPathPrefsBasename + "/"
  };
  inline const Glib::ustring kSchemaPathState {
    kSchemaPath + kSchemaPathStateBasename + "/"
  };
  inline const Glib::ustring kSchemaPathSound {
    kSchemaPathPrefs + kSchemaPathSoundBasename + "/"
  };
  inline const Glib::ustring kSchemaPathSoundThemes {
    kSchemaPathSound + kSchemaPathSoundThemesBasename + "/"
  };
  inline const Glib::ustring kSchemaPathShortcuts {
    kSchemaPathPrefs + kSchemaPathShortcutsBasename + "/"
  };

  /*
   * schema enum types
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
    kPendulumPhaseModeLeft  = 0,
    kPendulumPhaseModeRight = 1
  };

  /*
   * default values
   */
  inline constexpr  double    kDefaultVolume  =  75.0;
  inline constexpr  double    kMinVolume      =   0.0;
  inline constexpr  double    kMaxVolume      = 100.0;

  /*
   * .SettingsList keys
   */
  inline const Glib::ustring  kKeySettingsListEntries             {"entries"};
  inline const Glib::ustring  kKeySettingsListSelectedEntry       {"selected-entry"};

  /*
   * .preferences keys
   */
  inline const Glib::ustring  kKeyPrefsRestoreProfile             {"restore-profile"};
  inline const Glib::ustring  kKeyPrefsLinkSoundTheme             {"link-sound-theme"};
  inline const Glib::ustring  kKeyPrefsInputDeviceLatency         {"input-device-latency"};
  inline const Glib::ustring  kKeyPrefsPendulumAction             {"pendulum-action"};
  inline const Glib::ustring  kKeyPrefsPendulumPhaseMode          {"pendulum-phase-mode"};
  inline const Glib::ustring  kKeyPrefsMeterAnimation             {"meter-animation"};
  inline const Glib::ustring  kKeyPrefsAnimationSync              {"animation-sync"};
  inline const Glib::ustring  kKeyPrefsAudioBackend               {"audio-backend"};

#if HAVE_ALSA
  inline const Glib::ustring  kKeyPrefsAudioDeviceAlsa            {"audio-device-alsa"};
#endif
#if HAVE_OSS
  inline const Glib::ustring  kKeyPrefsAudioDeviceOss             {"audio-device-oss"};
#endif
#if HAVE_PULSEAUDIO
  inline const Glib::ustring  kKeyPrefsAudioDevicePulseaudio      {"audio-device-pulseaudio"};
#endif

  // map audio backend identifier (w/o kAudioBackendNone) to the corresponding
  // audio device settings key (e.g. kAudioBackendAlsa -> "audio-device-alsa")
  // and vice versa
  extern const std::map<settings::AudioBackend, Glib::ustring> kBackendToDeviceMap;
  extern const std::map<Glib::ustring, settings::AudioBackend> kDeviceToBackendMap;

  /*
   * .preferences.sound keys
   */
  inline const Glib::ustring  kKeySoundVolume                     {"volume"};
  inline const Glib::ustring  kKeySoundAutoAdjustVolume           {"auto-adjust-volume"};
  inline const Glib::ustring  kKeySoundThemeList                  {"theme-list"};

  /*
   * .preferences.sound.theme keys
   */
  inline const Glib::ustring  kKeySoundThemeTitle                 {"title"};

  /*
   * .preferences.sound.theme.parameters keys
   */
  inline const Glib::ustring  kKeySoundThemeTonePitch             {"tone-pitch"};
  inline const Glib::ustring  kKeySoundThemeToneTimbre            {"tone-timbre"};
  inline const Glib::ustring  kKeySoundThemeToneDetune            {"tone-detune"};
  inline const Glib::ustring  kKeySoundThemeToneAttack            {"tone-attack"};
  inline const Glib::ustring  kKeySoundThemeToneAttackShape       {"tone-attack-shape"};
  inline const Glib::ustring  kKeySoundThemeToneHold              {"tone-hold"};
  inline const Glib::ustring  kKeySoundThemeToneHoldShape         {"tone-hold-shape"};
  inline const Glib::ustring  kKeySoundThemeToneDecay             {"tone-decay"};
  inline const Glib::ustring  kKeySoundThemeToneDecayShape        {"tone-decay-shape"};
  inline const Glib::ustring  kKeySoundThemePercussionCutoff      {"percussion-cutoff"};
  inline const Glib::ustring  kKeySoundThemePercussionAttack      {"percussion-attack"};
  inline const Glib::ustring  kKeySoundThemePercussionAttackShape {"percussion-attack-shape"};
  inline const Glib::ustring  kKeySoundThemePercussionHold        {"percussion-hold"};
  inline const Glib::ustring  kKeySoundThemePercussionHoldShape   {"percussion-hold-shape"};
  inline const Glib::ustring  kKeySoundThemePercussionDecay       {"percussion-decay"};
  inline const Glib::ustring  kKeySoundThemePercussionDecayShape  {"percussion-decay-shape"};
  inline const Glib::ustring  kKeySoundThemeMix                   {"mix"};
  inline const Glib::ustring  kKeySoundThemePan                   {"pan"};
  inline const Glib::ustring  kKeySoundThemeVolume                {"volume"};

  /*
   * .preferences.shortcuts keys
   */
  inline const Glib::ustring  kKeyShortcutsQuit                   {"quit"};
  inline const Glib::ustring  kKeyShortcutsShowPrimaryMenu        {"show-primary-menu"};
  inline const Glib::ustring  kKeyShortcutsShowProfiles           {"show-profiles"};
  inline const Glib::ustring  kKeyShortcutsShowPreferences        {"show-preferences"};
  inline const Glib::ustring  kKeyShortcutsShowShortcuts          {"show-shortcuts"};
  inline const Glib::ustring  kKeyShortcutsShowAbout              {"show-about"};
  inline const Glib::ustring  kKeyShortcutsShowHelp               {"show-help"};
  inline const Glib::ustring  kKeyShortcutsShowPendulum           {"show-pendulum"};
  inline const Glib::ustring  kKeyShortcutsFullScreen             {"full-screen"};
  inline const Glib::ustring  kKeyShortcutsStart                  {"start"};
  inline const Glib::ustring  kKeyShortcutsVolumeIncrease1        {"volume-increase-1"};
  inline const Glib::ustring  kKeyShortcutsVolumeDecrease1        {"volume-decrease-1"};
  inline const Glib::ustring  kKeyShortcutsVolumeIncrease10       {"volume-increase-10"};
  inline const Glib::ustring  kKeyShortcutsVolumeDecrease10       {"volume-decrease-10"};
  inline const Glib::ustring  kKeyShortcutsVolumeMute             {"volume-mute"};
  inline const Glib::ustring  kKeyShortcutsTempoIncrease1         {"tempo-increase-1"};
  inline const Glib::ustring  kKeyShortcutsTempoDecrease1         {"tempo-decrease-1"};
  inline const Glib::ustring  kKeyShortcutsTempoIncrease10        {"tempo-increase-10"};
  inline const Glib::ustring  kKeyShortcutsTempoDecrease10        {"tempo-decrease-10"};
  inline const Glib::ustring  kKeyShortcutsTempoQuickSet          {"tempo-quick-set"};
  inline const Glib::ustring  kKeyShortcutsTempoTap               {"tempo-tap"};
  inline const Glib::ustring  kKeyShortcutsMeterEnabled           {"meter-enabled"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple2     {"meter-select-simple-2"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple3     {"meter-select-simple-3"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectSimple4     {"meter-select-simple-4"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound2   {"meter-select-compound-2"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound3   {"meter-select-compound-3"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCompound4   {"meter-select-compound-4"};
  inline const Glib::ustring  kKeyShortcutsMeterSelectCustom      {"meter-select-custom"};
  inline const Glib::ustring  kKeyShortcutsTrainerEnabled         {"trainer-enabled"};
  inline const Glib::ustring  kKeyShortcutsPendulumTogglePhase    {"pendulum-toggle-phase"};

  /*
   * .state keys
   */
  inline const Glib::ustring  kKeyStateFirstLaunch                {"first-launch"};
  inline const Glib::ustring  kKeyStateProfileSelect              {"profile-select"};
  inline const Glib::ustring  kKeyStateShowPendulum               {"show-pendulum"};

}//namespace settings

#include "SettingsList.h"
#include "SoundTheme.h"

namespace settings {
  /*
   * Access Gio::Settings or SettingLists of the application
   */
  Glib::RefPtr<Gio::Settings> settings();
  Glib::RefPtr<Gio::Settings> preferences();
  Glib::RefPtr<Gio::Settings> sound();
  Glib::RefPtr<SettingsList<SoundTheme>> soundThemes();
  Glib::RefPtr<Gio::Settings> shortcuts();
  Glib::RefPtr<Gio::Settings> state();

}//namespace settings
#endif//GMetronome_Settings_h
