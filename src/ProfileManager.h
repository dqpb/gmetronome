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

#ifndef GMetronome_ProfileManager_h
#define GMetronome_ProfileManager_h

#include "ProfileIOBase.h"
#include <sigc++/sigc++.h>
#include <memory>

class ProfileManager {

public:

  ProfileManager(std::unique_ptr<ProfileIOBase> ptr = nullptr);

  ProfileManager(ProfileManager&& pmgr);

  ~ProfileManager();

  void setIOModule(std::unique_ptr<ProfileIOBase> ptr);

  Profile::Primer newProfile(const Profile::Header& header = {},
                             const Profile::Content& content = {});

  void deleteProfile(const Profile::Identifier& id);

  std::vector<Profile::Primer> profileList();

  Profile getProfile(const Profile::Identifier& id);

  void setProfile(const Profile::Identifier& id, const Profile& profile);

  Profile::Content getProfileContent(const Profile::Identifier& id);

  void setProfileContent(const Profile::Identifier& id, const Profile::Content& content);

  Profile::Header getProfileHeader(const Profile::Identifier& id);

  void setProfileHeader(const Profile::Identifier& id, const Profile::Header& header);

  void reorderProfiles(const std::vector<Profile::Identifier>& order);

  sigc::signal<void> signal_changed()
  { return signal_changed_; }

private:
  sigc::signal<void> signal_changed_;
  std::unique_ptr<ProfileIOBase> io_;
};

#endif//GMetronome_ProfileManager_h
