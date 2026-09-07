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
#include "Settings.h"

#include <glibmm/ustring.h>
#include <iterator>

#ifndef NDEBUG
# include <iostream>
#endif

SoundThemeManager::SoundThemeManager(std::unique_ptr<ListStoreType> store,
                                     std::unique_ptr<ListStoreType> preset_store) noexcept
  : store_(std::move(store)),
    preset_store_(std::move(preset_store))
{
  selected_connection_ =
    settings::sound()->signal_changed(settings::kKeySoundThemeSelect).connect(
      [this] (const Glib::ustring&) { signal_selected_.emit(selected()); });
}

SoundThemeManager::~SoundThemeManager()
{
  selected_connection_.disconnect();
}

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
  if (id == kEmptyIdentifier)
    return {};

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
    // Find the next id to be selected
    Identifier next_id = id;
    if (id == selected())
    {
      if (const auto [list, it, result] = find(id); result != SearchResult::kNotFound)
      {
        if (std::next(it) != list.end())
          next_id = std::next(it)->id;
        else if (it != list.begin())
          next_id = std::prev(it)->id;
        else
          next_id = kDefaultIdentifier;
      }
    }
    if (auto result = store_->remove(id))
    {
      signal_removed_.emit(id);
      select(next_id);
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

bool SoundThemeManager::select(const Identifier& id)
{
  if (id == selected())
    return true;

  if (id != kEmptyIdentifier) {
    if (const auto& [list, it, result] = find(id); result == SearchResult::kNotFound)
    {
#ifndef NDEBUG
      std::cerr << "SoundThemeManager: Selected id '" << id << "' not found." << std::endl;
#endif
      return false;
    }
  }
  settings::sound()->set_string(settings::kKeySoundThemeSelect, id); // applies and emits
  return true;
}

auto SoundThemeManager::selected() const -> Identifier
{
  settings::sound()->apply();
  return settings::sound()->get_string(settings::kKeySoundThemeSelect);
}

auto SoundThemeManager::find(const Identifier& id)
  -> std::tuple<PrimerList, PrimerList::iterator, SearchResult>
{
  if (preset_store_) {
    if (auto result = preset_store_->list()) {
      auto it = std::find_if(result->begin(), result->end(),
                             [&id] (const auto& primer) { return (primer.id == id); });
      if (it != result->end())
        return {std::move(*result), it, SearchResult::kPreset};
    }
  }
  if (store_) {
    if (auto result = store_->list()) {
      auto it = std::find_if(result->begin(), result->end(),
                             [&id] (const auto& primer) { return (primer.id == id); });
      if (it != result->end())
        return {std::move(*result), it, SearchResult::kCustom};
    }
  }
  return {{}, {}, SearchResult::kNotFound};
}
