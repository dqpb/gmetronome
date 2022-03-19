/*
 * Copyright (C) 2020 The GMetronome Team
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "Application.h"
#include <glibmm/i18n.h>
#include <iostream>

int onHandleLocalOptions(const Glib::RefPtr<Glib::VariantDict>& options_dict)
{
  if(bool display_version;
     options_dict->lookup_value("version", display_version))
  {
    if (display_version)
      std::cout << PACKAGE_NAME << " " << PACKAGE_VERSION << std::endl;

    return 0;
  }
  // continue normal processing
  return -1;
}

int main (int argc, char *argv[])
{
  bindtextdomain(GETTEXT_PACKAGE, PGRM_LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  auto app = Application::create();

  app->add_main_option_entry(Gio::Application::OPTION_TYPE_BOOL, "version");

  app->signal_handle_local_options()
    .connect(&onHandleLocalOptions,true);

  return app->run(argc, argv);
}
