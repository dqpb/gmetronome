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

#include "ProfileManager.h"
#include <glib.h>

ProfileManager::ProfileManager(std::unique_ptr<ProfileIOBase> ptr)
  : io_(std::move(ptr))
{}

ProfileManager::ProfileManager(ProfileManager&& pmgr)
  : io_(std::move(pmgr.io_))
{}

ProfileManager::~ProfileManager()
{}

void ProfileManager::setIOModule(std::unique_ptr<ProfileIOBase> ptr)
{
  io_ = std::move(ptr);
  signal_changed_.emit();
}

Profile::Primer ProfileManager::newProfile(const Profile::Header& header,
                                           const Profile::Content& content)
{
  gchar* id = g_uuid_string_random();
  Profile p { header, content };
  io_->store(id, p);
  signal_changed_.emit();
  return {id, p.header};
}

void ProfileManager::deleteProfile(Profile::Identifier id)
{
  io_->remove(id);
  signal_changed_.emit();
}

std::vector<Profile::Primer> ProfileManager::profileList()
{
  return io_->list();
}

Profile ProfileManager::getProfile(Profile::Identifier id)
{
  return io_->load(id);
}

void ProfileManager::setProfile(Profile::Identifier id, const Profile& profile)
{
  io_->store(id, profile);
  signal_changed_.emit();
}

Profile::Content ProfileManager::getProfileContent(Profile::Identifier id)
{
  return getProfile(id).content;
}

void ProfileManager::setProfileContent(Profile::Identifier id, const Profile::Content& content)
{
  auto p = getProfile(id);
  p.content = content;
  setProfile(id, p);
}

Profile::Header ProfileManager::getProfileHeader(Profile::Identifier id)
{
  return getProfile(id).header;
}

void ProfileManager::setProfileHeader(Profile::Identifier id, const Profile::Header& header)
{
  auto p = getProfile(id);
  p.header = header;
  setProfile(id, p);
}

void ProfileManager::reorderProfiles(const std::vector<Profile::Identifier>& order)
{
  io_->reorder(order);
  signal_changed_.emit();
}
