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
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <map>

const Message& getDefaultMessage(MessageIdentifier id)
{
  static const std::map<MessageIdentifier, Message> msg_map = {
    {
      MessageIdentifier::kGenericError,
      {
        MessageCategory::kError,
        C_("Message", "Oops! Something went wrong."),

        Glib::ustring::compose(
          //The following parameters will be replaced:
          // %1 - localized application name
          // %2 - URL of the project's issues page
          C_("Message", "%1 has encountered an unknown error. Please check the details below. "
             "Since this error might be due to a bug in the software package, you can help us to "
             "improve %1 and file a bug report on the project's <a href=\"%2\">issues page</a>."),
          Glib::get_application_name(),
          PACKAGE_BUGREPORT),
        ""
      }
    },
    {
      MessageIdentifier::kAudioError,
      {
        MessageCategory::kError,
        C_("Message", "Audio problem"),
        C_("Message", "An audio related error occured. "
           "Please check the audio configuration in the preferences dialog and try again."),
        ""
      }
    }
  };

  return msg_map.at(id);
}
