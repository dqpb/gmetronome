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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ProfileManager.h"
#include "Error.h"
#include <glib.h>

#ifndef NDEBUG
# include <iostream>
#endif

void ProfileManager::setListStore(std::unique_ptr<ListStoreType> ptr)
{
  store_ = std::move(ptr);
  signal_changed_.emit();
}

auto ProfileManager::newProfile(const Profile& profile) -> Primer
{
  if (store_) {
    gchar* uuid = g_uuid_string_random();
    Profile::Identifier id {uuid};
    g_free(uuid);

    if (auto result = store_->store(id, profile))
    {
      signal_changed_.emit();
      return {id, profile.header};
    }
    else
      printError("Failed to store new profile '" + profile.header.title + "'.", result.error());
  }
  return {};
}

void ProfileManager::deleteProfile(const Profile::Identifier& id)
{
  if (store_) {
    if (auto result = store_->remove(id))
      signal_changed_.emit();
    else
      printError("Failed to remove profile '" + id + "'.", result.error());
  }
}

auto ProfileManager::profileList() -> std::vector<Primer>
{
  if (store_) {
    if (auto result = store_->list())
      return *result;
    else
      printError("ProfileManager: Failed to fetch profile list.", result.error());
  }
  return {};
}

Profile ProfileManager::getProfile(const Profile::Identifier& id)
{
  if (auto result = store_->load(id))
    return *result;
  else
    printError("Failed to load profile '" + id + "'.", result.error());

  return {};
}

void ProfileManager::setProfile(const Profile::Identifier& id,
                                const Profile& profile)
{
  if (auto result = store_->store(id, profile))
    signal_changed_.emit();
  else
    printError("Failed to store profile '" + profile.header.title + "'.", result.error());
}

void ProfileManager::setProfileContent(const Profile::Identifier& id,
                                       const Profile::Content& content)
{
  auto p = getProfile(id);
  p.content = content;
  setProfile(id, p);
}

void ProfileManager::setProfileHeader(const Profile::Identifier& id,
                                      const Profile::Header& header)
{
  auto p = getProfile(id);
  p.header = header;
  setProfile(id, p);
}

void ProfileManager::reorderProfiles(const std::vector<Profile::Identifier>& order)
{
  if (auto result = store_->reorder(order))
    signal_changed_.emit();
  else
    printError("Failed to reorder profiles.", result.error());
}

void ProfileManager::printError(const std::string& msg, const ListStoreType::Error& e)
{
#ifndef NDEBUG
  std::cerr << "ProfileManager: " << msg;
  if (!e.what.empty())
    std::cerr << " (" << e.what << ")";
  std::cerr << std::endl;
  if (!e.detail.empty())
    std::cerr << "ProfileManager: Detail: " << e.detail << std::endl;
#endif
}
