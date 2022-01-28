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

#ifndef GMetronome_ProfileIOLocalXml_h
#define GMetronome_ProfileIOLocalXml_h

#include "ProfileIOBase.h"
#include <gtkmm.h>
#include <map>

class ProfileIOLocalXml : public ProfileIOBase
{
public:
  ProfileIOLocalXml(Glib::RefPtr<Gio::File> file = defaultFile());

  ~ProfileIOLocalXml() override;

  std::vector<Profile::Primer> list() override;

  Profile load(Profile::Identifier id) override;

  void store(Profile::Identifier id, const Profile& profile) override;

  void reorder(const std::vector<Profile::Identifier>& order) override;

  void remove(Profile::Identifier id) override;

  void flush() override;

public:
  static Glib::RefPtr<Gio::File> defaultFile();

  using ProfileMap = std::map<Profile::Identifier, Profile>;

  const ProfileMap& profileMap() const
    { return pmap_; }

protected:
  Glib::RefPtr<Gio::File> file_;
  ProfileMap pmap_;
  std::vector<Profile::Identifier> porder_;
  bool pending_import_;
  bool import_error_;
  bool pending_export_;
  bool export_error_;

  void importProfiles();
  void exportProfiles();
};

#endif//GMetronome_ProfileIOLocalXml_h
