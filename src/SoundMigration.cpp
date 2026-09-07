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

#ifndef NDEBUG
# include <iostream>
#endif

namespace sound_migration {

  namespace {
    void convert(SoundTheme& sound)
    {
      // not implemented yet
      // TODO: convert volume in percent to dB
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

  bool validate()
  {
    auto settings_list = settings::soundThemes();
    auto list_store = std::make_unique<SoundThemeListStoreXML>(file::userSoundsPath());

    if (!settings_list || !list_store)
      return false;

    const auto id_list = settings_list->list(false); // custom only

    if (const auto primer_list = list_store->list())
      return std::equal(id_list.begin(), id_list.end(),
                        primer_list->begin(), primer_list->end(),
                        [] (const auto& id, const auto& primer) {
                          return id == primer.id;
                        });
    else
      return false;
  }

} // namespace sound_migration
