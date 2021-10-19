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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <glibmm/ustring.h>

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
 * org.gmetronome.preferences keys
 */
const Glib::ustring  kKeyPrefsVolume                    {"volume"};
const Glib::ustring  kKeyPrefsRestoreProfile            {"restore-profile"};
const Glib::ustring  kKeyPrefsMeterAnimation            {"meter-animation"};
const Glib::ustring  kKeyPrefsAnimationSync             {"animation-sync"};
const Glib::ustring  kKeyPrefsSoundHighFrequency        {"sound-high-frequency"};
const Glib::ustring  kKeyPrefsSoundHighVolume           {"sound-high-volume"};
const Glib::ustring  kKeyPrefsSoundMidFrequency         {"sound-mid-frequency"};
const Glib::ustring  kKeyPrefsSoundMidVolume            {"sound-mid-volume"};
const Glib::ustring  kKeyPrefsSoundLowFrequency         {"sound-low-frequency"};
const Glib::ustring  kKeyPrefsSoundLowVolume            {"sound-low-volume"};
const Glib::ustring  kKeyPrefsAudioBackend              {"audio-backend"};

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

#endif//__SETTINGS_H__
