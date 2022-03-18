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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "About.h"
#include <glibmm/i18n.h>

GMetronomeAboutDialog::GMetronomeAboutDialog(bool use_header_bar)
  : AboutDialog(use_header_bar)
{
  // year of last commit
  static const int year = 2022;

  set_program_name(Glib::get_application_name());
  set_version(VERSION);
  set_license_type(Gtk::LICENSE_GPL_3_0);
  set_authors({"dqpb <dqpb@mailbox.org>, 2020-2022"});

  //Put one translator per line, in the form
  //NAME <EMAIL>, YEAR1, YEAR2
  set_translator_credits(C_("About dialog", "translator-credits"));

  auto copyright = Glib::ustring::compose(
    //The following parameters will be replaced:
    // %1 - year of the last commit
    // %2 - localized application name
    C_("About dialog", "Copyright © 2020-%1 The %2 Team"),
    year, Glib::get_application_name());

  set_copyright(copyright);
  set_website(PACKAGE_URL);
  set_website_label( C_("About dialog", "Website") );
  set_logo_icon_name(PACKAGE_ID);
}
