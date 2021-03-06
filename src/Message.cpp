/*
 * Copyright (C) 2021 The GMetronome Team
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

#include "Message.h"

const Message kGenericErrorMessage
{
  MessageCategory::kError,
  "Oops! Something went wrong.",
  PACKAGE_NAME " has encountered an unknown error. Please check the "
  "details below. Since this error might be due to a bug in the software "
  "package, you can help us to improve " PACKAGE_NAME " and file a bug "
  "report on the <a href=\"" PACKAGE_URL "\">project page</a>.",
  ""
};

const Message kAudioErrorMessage
{
  MessageCategory::kError,
  "Audio problem",
  "An audio related error occured. For further information see the details below. "
  "Please check the audio configuration in the preferences dialog and try again.",
  ""
};

const Message kAudioBackendErrorMessage
{
  MessageCategory::kError,
  "Audio backend error",
  "An error occured in the audio backend. Please try again or "
  "check the audio configuration in the preferences dialog.",
  ""
};

const Message kSoundThemeLoadingErrorMessage
{
  MessageCategory::kWarning,
  "Sound Theme",
  "Some problems occured loading the selected sound theme. "
  "The sound theme may have been removed or is otherwise inaccessible. "
  "Please check the sound configuration in the preferences dialog.",
  ""
};
