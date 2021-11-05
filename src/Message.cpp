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

#include "Message.h"
#include "config.h"

const Message kGenericErrorMessage
{
  MessageCategory::kError,
  "Oops! Something went wrong.",
  PACKAGE_NAME " has encountered an unknown error. Please check the details below "
  "and file a bug report on the <a href=\"" PACKAGE_URL "\">project page</a> to help "
  "to improve " PACKAGE_NAME ".",
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

