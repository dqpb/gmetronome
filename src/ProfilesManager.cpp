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

#include "ProfilesManager.h"
#include <glib.h>

ProfilesManager::ProfilesManager(std::unique_ptr<ProfilesIOBase> ptr)
  : io_(std::move(ptr))
{}

ProfilesManager::ProfilesManager(ProfilesManager&& pmgr)
  : io_(std::move(pmgr.io_))
{}

ProfilesManager::~ProfilesManager()
{}
  
void ProfilesManager::setIOModule(std::unique_ptr<ProfilesIOBase> ptr)
{
  io_ = std::move(ptr);
  signal_changed_.emit();
}
  
Profile::Primer ProfilesManager::newProfile(const Profile::Header& header,
                                            const Profile::Content& content)
{
  gchar* id = g_uuid_string_random();
  Profile p { header, content };
  io_->store(id, p);
  signal_changed_.emit();
  return {id, p.header};
}

void ProfilesManager::deleteProfile(Profile::Identifier id)
{
  io_->remove(id);
  signal_changed_.emit();
}
  
std::vector<Profile::Primer> ProfilesManager::profileList()
{
  return io_->list();
}
  
Profile ProfilesManager::getProfile(Profile::Identifier id)
{
  return io_->load(id);
}
  
void ProfilesManager::setProfile(Profile::Identifier id, const Profile& profile)
{
  io_->store(id, profile);
  signal_changed_.emit();
}

Profile::Content ProfilesManager::getProfileContent(Profile::Identifier id)
{
  return getProfile(id).content;
}
  
void ProfilesManager::setProfileContent(Profile::Identifier id, const Profile::Content& content)
{
  auto p = getProfile(id);
  p.content = content;
  setProfile(id, p);
}

Profile::Header ProfilesManager::getProfileHeader(Profile::Identifier id)
{
  return getProfile(id).header;
}
  
void ProfilesManager::setProfileHeader(Profile::Identifier id, const Profile::Header& header)
{
  auto p = getProfile(id);
  p.header = header;
  setProfile(id, p);  
}

void ProfilesManager::reorderProfiles(const std::vector<Profile::Identifier>& order)
{
  io_->reorder(order);
  signal_changed_.emit();
}
