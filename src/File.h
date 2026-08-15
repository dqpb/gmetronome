/*
 * Copyright (C) 2026 The GMetronome Team
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

#ifndef GMetronome_File_h
#define GMetronome_File_h

#include <string>
#include <vector>

namespace file {

  // E.g. ~/.share/gmetronome
  const std::string& userDataDirectory();

  // E.g. /usr/local/share/gmetronome, /usr/share/gmetronome, ...
  const std::vector<std::string>& systemDataDirectories();

  inline const std::string kProfilesFileName = "profiles.xml";
  inline const std::string kSoundThemesFileName = "sound_themes.xml";
  inline const std::string kSoundThemesPresetsFileName = "sound_themes_presets.xml";

  // E.g. ~/.share/gmetronome/profiles.xml
  const std::string& userProfilesPath();

  // E.g. ~/.share/gmetronome/sound_themes.xml
  const std::string& userSoundThemesPath();

  // Search for a resource file beginning with user data directory (~/share/gmetronome)
  // followed by system-wide data direcories (/usr/local/share/gmetronome, ...)
  std::string lookupResourcePath(const std::string& filename);

  // Search and cache profiles file path.
  const std::string& lookupProfilesPath();
  // Search and cache sound themes file path.
  const std::string& lookupSoundThemesPath();
  // Search and cache sound themes presets file path.
  const std::string& lookupSoundThemesPresetsPath();

}//namespace file
#endif//GMetronome_File_h
