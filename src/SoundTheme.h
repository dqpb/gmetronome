/*
 * Copyright (C) 2022 The GMetronome Team
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

#ifndef GMetronome_SoundTheme_h
#define GMetronome_SoundTheme_h

#include "Synthesizer.h"
#include <string>
#include <glibmm/i18n.h>

struct SoundTheme
{
  using Identifier = std::string;

  // Default title for new sound themes
  static inline const std::string kDefaultTitle = C_("Sound theme", "New Sound Theme");

  // Placeholder title for untitled sound themes
  static inline const std::string kDefaultTitlePlaceholder = C_("Sound theme", "Untitled");

  // Title of duplicated sound themes, %1 will be replaced by the old title
  static inline const std::string kDefaultTitleDuplicate = C_("Sound theme", "%1 (copy)");

  // Default description for new sound themes
  static inline const std::string kDefaultDescription = "";

  struct Header
  {
    std::string title = kDefaultTitle;
    std::string description = kDefaultDescription;
  };

  struct Content
  {
    audio::SoundParameters strong_params;
    audio::SoundParameters mid_params;
    audio::SoundParameters weak_params;
  };

  Header header;
  Content content;
};

#endif//GMetronome_SoundTheme_h
