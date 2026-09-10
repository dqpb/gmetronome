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

#include "SoundMigration.h"
#include "Settings.h"
#include "File.h"
#include "SoundThemeListStoreXML.h"

#include <glibmm/fileutils.h>
#include <memory>
#include <cctype>

#ifndef NDEBUG
# include <iostream>
#endif

namespace sound_migration {

  namespace {
    void convert(SoundTheme& sound)
    {
      // Convert volume levels (in percent) to dB. Up until now we mapped the
      // volume cubically to get the gain, so we apply that mapping too.

      sound.content.weak_params.gain = audio::Decibel::fromLinear(
        std::pow(sound.content.weak_params.gain.value() / 100.0, 3.0));

      sound.content.mid_params.gain = audio::Decibel::fromLinear(
        std::pow(sound.content.mid_params.gain.value() / 100.0, 3.0));

      sound.content.strong_params.gain = audio::Decibel::fromLinear(
        std::pow(sound.content.strong_params.gain.value() / 100.0, 3.0));
    }

    // Check RFC 4122 canonical form: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    bool isUuid(const std::string& s) {
      if (s.size() != 36)
        return false;

      for (size_t i = 0; i < s.size(); ++i) {
        switch (i) {
        case 8:
        case 13:
        case 18:
        case 23:
          if (s[i] != '-') return false;
          break;
        default:
          if (!std::isxdigit(static_cast<unsigned char>(s[i])))
            return false;
          break;
        }
      }
      return true;
    }
  }//unnamed namespace

  bool check()
  {
    if (Glib::file_test(file::userSoundsPath(), Glib::FILE_TEST_EXISTS))
      return false;

    try {
      if (auto settings_list = settings::soundThemes()) {
        if (auto sound_ids = settings_list->list(false); !sound_ids.empty())
          return true;
      }
    } catch (...) {
      // ignore
    }
    return false;
  }

  bool transfer()
  {
    try {
      if (auto settings_list = settings::soundThemes())
      {
        auto list_store = std::make_unique<SoundThemeListStoreXML>(file::userSoundsPath());
        for (const auto& id : settings_list->list(false)) {
          try {
            if (!isUuid(id)) // ignore old presets
              continue;
            SoundTheme sound = settings_list->get(id);
#ifndef NDEBUG
            std::cerr << "Sound migration: Transfer '" << sound.header.title << "'." << std::endl;
#endif
            sound_migration::convert(sound);
            list_store->store(id, sound);
          }
          catch (...) {
#ifndef NDEBUG
            std::cerr << "Sound migration: Failed to transfer sound theme '"
                      << id << "'." << std::endl;
#endif
            continue;
          }
        }
        if (list_store->flush())
        {
          const auto selected_id = settings_list->selected();
#ifndef NDEBUG
          std::cout << "Sound migration: Select theme '" << selected_id << "'" << std::endl;
#endif
          settings::sound()->set_string(settings::kKeySoundThemeSelect, selected_id);
          settings::sound()->apply();
          return true;
        }
      }
    }
    catch(const GMetronomeError& e) {
#ifndef NDEBUG
      std::cerr << "Sound migration: Error during sound theme transfer. " << std::endl;
      std::cerr << "Sound migration: Reason: " << e.what() << std::endl;
#endif
    }
    catch (...) {
#ifndef NDEBUG
      std::cerr << "Sound migration: Unknown error during sound theme transfer. " << std::endl;
#endif
    }
    return false;
  }

} // namespace sound_migration
