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

#ifndef GMetronome_SoundThemeManager_h
#define GMetronome_SoundThemeManager_h

#include "SoundTheme.h"
#include "ListStore.h"
#include "Meter.h"

#include <sigc++/sigc++.h>

#include <memory>
#include <utility>
#include <optional>

class SoundThemeManager {
public:
  // Type aliases
  using Header = SoundTheme::Header;
  using Content = SoundTheme::Content;
  using Identifier = SoundTheme::Identifier;
  using ListStoreType = ListStore<SoundTheme, Identifier, Header>;
  using Primer = ListStoreType::Primer;

  struct Patch : public ListStoreType::Patch
  {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<audio::SoundParameters> strong_params;
    std::optional<audio::SoundParameters> mid_params;
    std::optional<audio::SoundParameters> weak_params;

    void apply(SoundTheme& theme) const override
      {
        if (title) theme.header.title = *title;
        if (description) theme.header.description = *description;
        if (strong_params) theme.content.strong_params = *strong_params;
        if (mid_params) theme.content.mid_params = *mid_params;
        if (weak_params) theme.content.weak_params = *weak_params;
      }
  };

public:
  // Construction and destruction
  SoundThemeManager(std::unique_ptr<ListStoreType> store = nullptr,
                    std::unique_ptr<ListStoreType> preset_store = nullptr) noexcept
    : store_(std::move(store)), preset_store_(std::move(preset_store))
    { /* nothing */ }

  SoundThemeManager(SoundThemeManager&& other) = default;
  SoundThemeManager& operator=(SoundThemeManager&& other) = default;
  ~SoundThemeManager() = default;

  // Interface
  void setStore(std::unique_ptr<ListStoreType> store);
  void setPresetStore(std::unique_ptr<ListStoreType> store);

  std::vector<Primer> list() noexcept;
  std::vector<Primer> presets() noexcept;

  std::optional<SoundTheme> get(const Identifier& id);

  std::optional<Primer> create(const SoundTheme& theme);
  std::optional<Primer> create(const Header& header = {}, const Content& content = {})
    { return create(SoundTheme{header, content}); }
  std::optional<Primer> create(const Content& content)
    { return create(SoundTheme{{}, content}); }

  bool remove(const Identifier& id);

  bool update(const Identifier& id, const Patch& patch);

  bool reorder(const std::vector<Identifier>& order);

  // selection state currently not supported
  bool select(const Identifier& id) = delete;
  bool unselect() = delete;
  const Identifier& selected() const = delete;

  // Signals
  sigc::signal<void(const Identifier&)> signalCreated()
    { return signal_created_; }
  sigc::signal<void(const Identifier&)> signalRemoved()
    { return signal_removed_; }
  sigc::signal<void(const Identifier&, const Patch&)> signalUpdated()
    { return signal_updated_; }
  sigc::signal<void(const std::vector<Identifier>&)> signalReordered()
    { return signal_reordered_; }
  sigc::signal<void> signalStoreChanged()
    { return signal_store_changed_; }
  sigc::signal<void> signalPresetStoreChanged()
    { return signal_preset_store_changed_; }

private:
  // Signals
  sigc::signal<void(const Identifier&)> signal_created_;
  sigc::signal<void(const Identifier&)> signal_removed_;
  sigc::signal<void(const Identifier&, const Patch&)> signal_updated_;
  sigc::signal<void(const std::vector<Identifier>&)> signal_reordered_;
  sigc::signal<void> signal_store_changed_;
  sigc::signal<void> signal_preset_store_changed_;

  // Underlying list stores
  std::unique_ptr<ListStoreType> store_;
  std::unique_ptr<ListStoreType> preset_store_;
};

#endif//GMetronome_SoundThemeManager_h
