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

#include <memory>
#include <sigc++/sigc++.h>

class SoundThemeManager {
public:
  // Type aliases
  using Header = SoundTheme::Header;
  using Content = SoundTheme::Content;
  using Identifier = SoundTheme::Identifier;
  using ListStoreType = ListStore<SoundTheme, Identifier, Header>;
  using Primer = ListStoreType::Primer;

public:
  // Construction and destruction
  SoundThemeManager(std::unique_ptr<ListStoreType> store = nullptr,
                    std::unique_ptr<ListStoreType> preset_store = nullptr);
  SoundThemeManager(SoundThemeManager&& other) = default;
  SoundThemeManager& operator=(SoundThemeManager&& other) = default;
  ~SoundThemeManager();

  // Interface
  void setStore(std::unique_ptr<ListStoreType> store);
  void setPresetStore(std::unique_ptr<ListStoreType> store);

  std::vector<Primer> list();
  std::vector<Primer> presets();

  SoundTheme get(const Identifier& id);
  SoundTheme getPreset(const Identifier& id);

  Primer create(const SoundTheme& theme);
  Primer create(const Content& content);
  Primer create(const Header& header = {}, const Content& content = {});

  void remove(const Identifier& id);

  void update(const Identifier& id, const SoundTheme& theme,
              const AccentFlags& flags = kAccentMaskAll);
  void update(const Identifier& id, const Content& content,
              const AccentFlags& flags = kAccentMaskAll);
  void update(const Identifier& id, const Header& header)
    { update(id, {header, {}}, kAccentMaskNone); }

  void reorder(const std::vector<Identifier>& order);

  sigc::signal<void> signal_store_changed()
    { return signal_store_changed_; }

private:
  // Signals
  sigc::signal<void(bool, const AccentFlags&)> signal_updated_;
  sigc::signal<void> signal_removed_;
  sigc::signal<void> signal_created_;
  sigc::signal<void> signal_reordered_;
  sigc::signal<void> signal_store_changed_;
  sigc::signal<void> signal_preset_store_changed_;

  // Underlying list stores
  std::unique_ptr<ListStoreType> store_;
  std::unique_ptr<ListStoreType> preset_store_;
};

#endif//GMetronome_SoundThemeManager_h
