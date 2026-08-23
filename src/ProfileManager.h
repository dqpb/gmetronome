/*
 * Copyright (C) 2020,2026 The GMetronome Team
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

#include "Profile.h"
#include "ListStore.h"

#include <sigc++/sigc++.h>
#include <memory>

class ProfileManager {
public:
  // Type aliases
  using ListStoreType = ListStore<Profile, Profile::Identifier, Profile::Header>;
  using Primer = ListStoreType::Primer;

public:
  ProfileManager(std::unique_ptr<ListStoreType> ptr = nullptr) : store_(std::move(ptr))
    { /* nothing */ }
  ProfileManager(ProfileManager&&) = default;
  ProfileManager& operator=(ProfileManager&&) = default;
  ~ProfileManager() = default;

  void setListStore(std::unique_ptr<ListStoreType> ptr);

  Primer newProfile(const Profile& profile = {});

  void deleteProfile(const Profile::Identifier& id);

  std::vector<Primer> profileList();

  Profile getProfile(const Profile::Identifier& id);
  Profile::Content getProfileContent(const Profile::Identifier& id)
    { return getProfile(id).content; }
  Profile::Header getProfileHeader(const Profile::Identifier& id)
    { return getProfile(id).header; }

  void setProfile(const Profile::Identifier& id, const Profile& profile);
  void setProfileContent(const Profile::Identifier& id, const Profile::Content& content);
  void setProfileHeader(const Profile::Identifier& id, const Profile::Header& header);

  void reorderProfiles(const std::vector<Profile::Identifier>& order);

  sigc::signal<void> signal_changed()
  { return signal_changed_; }

private:
  sigc::signal<void> signal_changed_;
  std::unique_ptr<ListStoreType> store_;

  void printError(const std::string& msg, const ListStoreType::Error& e = {});
};

#endif//GMetronome_ProfileManager_h
