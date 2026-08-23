/*
 * Copyright (C) 2026 The GMetronome Team
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

#include "SoundThemeManager.h"

#ifndef NDEBUG
# include <iostream>
#endif

void SoundThemeManager::setStore(std::unique_ptr<ListStoreType> store)
{
  store_ = std::move(store);
  signal_store_changed_.emit();
}

void SoundThemeManager::setPresetStore(std::unique_ptr<ListStoreType> store)
{
  preset_store_ = std::move(store);
  signal_preset_store_changed_.emit();
}

std::vector<SoundThemeManager::Primer> SoundThemeManager::list() noexcept
{
  if (store_) {
    if (auto result = store_->list())
      return *result;
    else {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Failed to fetch list "
                << "(" << result.error().what << ")." << std::endl;
#endif
    }
  }
  return {};
}

std::vector<SoundThemeManager::Primer> SoundThemeManager::presets() noexcept
{
  if (preset_store_) {
    if (auto result = preset_store_->list())
      return *result;
    else {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Failed to fetch preset list." << std::endl;
      std::cerr << "SoundThemeManager: Reason: " << result.error().what << std::endl;
#endif
    }
  }
  return {};
}

std::optional<SoundTheme> SoundThemeManager::get(const Identifier& id)
{
  if (preset_store_) {
    if (auto result = preset_store_->load(id))
      return *result;
  }
  if (store_) {
    if (auto result = store_->load(id))
      return *result;
  }
#ifndef NDEBUG
    std::cerr << "SoundThemeManager: Sound theme '"  << id << "' not found." << std::endl;
#endif
  return {};
}

std::optional<SoundThemeManager::Primer> SoundThemeManager::create(const SoundTheme& theme)
{
  if (store_) {
    gchar* uuid = g_uuid_string_random();
    SoundTheme::Identifier id {uuid};
    g_free(uuid);

    if (auto result = store_->store(id, theme))
    {
      signal_created_.emit(id);
      return {{id, theme.header}};
    }
    else {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Failed to store new sound theme "
                << "'" << theme.header.title << "'." << std::endl;
#endif
    }
  }
  return {};
}

bool SoundThemeManager::remove(const Identifier& id)
{
  if (store_) {
    if (auto result = store_->remove(id)) {
      signal_removed_.emit(id);
      return true;
    }
    else {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Failed to remove sound theme "
                << "'" << id << "'." << std::endl;
      std::cerr << "SoundThemeManager: Reason: " << result.error().what << std::endl;
#endif
    }
  }
  return false;
}

bool SoundThemeManager::update(const Identifier& id, const Patch& patch)
{
  if (store_) {
    if (auto result = store_->update(id, patch)) {
      signal_updated_.emit(id, patch);
      return true;
    }
    else {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Failed to update sound theme " << std::endl
                << "'" << id << "'." << std::endl;
      std::cerr << "SoundThemeManager: Reason: " << result.error().what << std::endl;
#endif
    }
  }
  return false;
}

bool SoundThemeManager::reorder(const std::vector<Identifier>& order)
{
  if (store_) {
    if (auto result = store_->reorder(order)) {
      signal_reordered_.emit(order);
      return true;
    }
    else {
#ifndef NDEBUG
    std::cerr << "SoundThemeManager: Failed to reorder sound themes." << std::endl;
    std::cerr << "SoundThemeManager: Reason" << result.error().what << std::endl;
#endif
    }
  }
  return false;
}
