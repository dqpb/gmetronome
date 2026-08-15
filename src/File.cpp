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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "File.h"

#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>

#include <iostream>

namespace file {

  namespace {
    std::vector<std::string> buildSystemDataDirectories()
    {
      std::vector<std::string> dirs;
      for (const auto& dir : Glib::get_system_data_dirs()) {
        dirs.push_back(Glib::build_filename(dir, PACKAGE));
      }
      return dirs;
    }
  }//unnamed namespace

  const std::string& userDataDirectory()
  {
    static const std::string dir = Glib::build_filename(Glib::get_user_data_dir(), PACKAGE);
    return dir;
  }

  const std::vector<std::string>& systemDataDirectories()
  {
    static const std::vector<std::string> dir = buildSystemDataDirectories();
    return dir;
  }

  const std::string& userProfilesPath()
  {
    static const std::string path = Glib::build_filename(userDataDirectory(), kProfilesFileName);
    return path;
  }

  const std::string& userSoundThemesPath()
  {
    static const std::string path = Glib::build_filename(userDataDirectory(), kSoundThemesFileName);
    return path;
  }

  std::string lookupResourceFile(const std::string& filename)
  {
    auto file = Glib::build_filename(userDataDirectory(), filename);

    if (Glib::file_test(file, Glib::FILE_TEST_EXISTS))
      return file;

    for (const auto& dir : systemDataDirectories())
    {
      file = Glib::build_filename(dir, filename);
      if (Glib::file_test(file, Glib::FILE_TEST_EXISTS))
        return file;
    }

    return std::string();
  }

  const std::string& lookupProfilesPath()
  {
    static const std::string path = lookupResourceFile(kProfilesFileName);
    return path;
  }
  const std::string& lookupSoundThemesPath()
  {
    static const std::string path = lookupResourceFile(kSoundThemesFileName);
    return path;
  }
  const std::string& lookupSoundThemesPresetsPath()
  {
    static const std::string path = lookupResourceFile(kSoundThemesPresetsFileName);
    return path;
  }
}//namspace file
