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

#ifndef GMetronome_SoundMigration_h
#define GMetronome_SoundMigration_h

namespace sound_migration {

  /**
   * Check if sounds should be transfered, i.e. if the user sounds file does
   * not already exists and at least one custom sound is defined in the settings list.
   */
  bool check();

  /**
   * Transfer all custom sound themes from the settings list to the user sound
   * file and copy the selected id to the new settings key.
   */
  bool transfer();

  /** Open the user sounds file and the settings list and compares the ids. */
  bool validate();

}//namespace sound_migration

#endif//GMetronome_SoundMigration_h
