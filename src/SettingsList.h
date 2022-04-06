/*
 * Copyright (C) 2022 The GMetronome Team
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

#ifndef GMetronome_SettingsList_h
#define GMetronome_SettingsList_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Settings.h"
#include "Error.h"

#include <giomm/settings.h>
#include <glib.h>
#include <string>
#include <algorithm>
#include <cassert>
#include <map>

#ifndef NDEBUG
# include <iostream>
#endif

// clients need to specialize this converter class template
template<typename ValueType>
struct SettingsListConverter
{
  static ValueType load(Glib::RefPtr<Gio::Settings> settings);
  static void store(const ValueType& value, Glib::RefPtr<Gio::Settings> settings);
};

template<typename ValueType>
class SettingsList : public Glib::Object {
public:
  using Identifier = Glib::ustring;
  using Converter = SettingsListConverter<ValueType>;

  static Glib::RefPtr<SettingsList<ValueType>>
  create(Glib::RefPtr<Gio::Settings> base_settings, std::string entry_schema_id)
    {
      return Glib::RefPtr(new SettingsList<ValueType>(base_settings, std::move(entry_schema_id)));
    }

public:
  SettingsList(Glib::RefPtr<Gio::Settings> base_settings, std::string entry_schema_id)
    : Glib::ObjectBase{"SettingsList"},
      entry_schema_id_{entry_schema_id},
      base_settings_{base_settings}
    {
      assert (base_settings_);
    }

  ValueType get(const Identifier& id)
    {
      auto settings = getCachedEntrySettings(id);
      if (settings)
        return Converter::load(settings);
      else
        throw GMetronomeError {"could not get Gio::Settings to access list entry"};
    }

  void update(const Identifier& id, const ValueType& value)
    {
      auto settings = getCachedEntrySettings(id);
      if (settings)
        Converter::store(value, settings);
      else
        throw GMetronomeError {"could not get Gio::Settings to access list entry"};
    }

  Identifier append(const ValueType& value)
    {
      gchar* uuid = g_uuid_string_random();
      Identifier id {uuid};
      g_free(uuid);

      auto settings = createEntrySettings(id);
      if (settings)
        Converter::store(value, settings);
      else
        throw GMetronomeError {"could not create Gio::Settings to append list entry"};

      auto list = entries();
      list.push_back(id);

      base_settings_->set_string_array(settings::kKeySettingsListEntries, list);
      return id;
    }

  void remove(const Identifier& id)
    {
      // we do not remove default entries
      auto children = defaults();
      if (std::find(children.begin(), children.end(), id) != children.end())
      {
#ifndef NDEBUG
        std::cerr << "SettingsList: ignoring attempt to remove default entry '" << id << "'"
                  << std::endl;
#endif
        return;
      }

      if (selected() == id)
        base_settings_->set_string(settings::kKeySettingsListSelectedEntry, "");

      auto list = entries();
      if (auto it = std::find(list.begin(), list.end(), id); it != list.end())
      {
        list.erase(it);
        base_settings_->set_string_array(settings::kKeySettingsListEntries, list);
      }

      if (auto settings = getCachedEntrySettings(id); settings)
      {
        settings->delay();
        resetRecursively(settings);
        settings->apply();
        removeEntrySettings(id);
      }
      else
      {
#ifndef NDEBUG
        std::cerr << "SettingsList: failed to remove entry '" << id << "' from gsettings backend"
                  << std::endl;
#endif
      }
    }

  void select(const Identifier& id)
    {
      if (id.empty())
        base_settings_->set_string(settings::kKeySettingsListSelectedEntry, "");
      else if (auto l = list(); std::find(l.begin(), l.end(), id) != l.end())
        base_settings_->set_string(settings::kKeySettingsListSelectedEntry, id);
#ifndef NDEBUG
      else
        std::cerr << "SettingsList: could not select entry '" << id << "' (not found)"
                  << std::endl;
#endif
    }

  Identifier selected()
    { return base_settings_->get_string(settings::kKeySettingsListSelectedEntry); }

  std::vector<Identifier> list()
    {
      std::vector<Identifier> list = entries();

      // Check if the list contains the default entries (child schemas) and insert
      // them in front of the list if necessary.
      // TODO: optimize to prevent unnecessary vector copies
      auto children = defaults();
      for (auto it = children.rbegin(); it != children.rend(); ++it)
      {
        if (std::find(list.begin(), list.end(), *it) == list.end())
          list.insert(list.begin(), std::move(*it));
      }
      // TODO: cache this list and monitor settings::kKeySettingsListEntries
      //       for changes
      return list;
    }

  std::vector<Identifier> entries()
    {
      return base_settings_->get_string_array(settings::kKeySettingsListEntries);
    }

  std::vector<Identifier> defaults() const
    { return base_settings_->list_children(); }

  Glib::RefPtr<Gio::Settings> settings(const Identifier& id)
    { return getCachedEntrySettings(id); }

  Glib::RefPtr<Gio::Settings> settings()
    { return base_settings_; }

private:
  std::string entry_schema_id_;
  Glib::RefPtr<Gio::Settings> base_settings_;
  std::map<Identifier, Glib::RefPtr<Gio::Settings>> entries_settings_;

  bool hasEntrySettings(const Identifier& id)
    {
      if (auto it = entries_settings_.find(id); it != entries_settings_.end())
        return true;
      else
        return false;
    }

  Glib::RefPtr<Gio::Settings> getCachedEntrySettings(const Identifier& id)
    {
      if (auto it = entries_settings_.find(id); it != entries_settings_.end())
        return it->second;
      else if (const auto& l = list(); std::find(l.begin(), l.end(), id) != l.end())
        return createEntrySettings(id);
      else
        return {};
    }

  Glib::RefPtr<Gio::Settings> createEntrySettings(const Identifier& id)
    {
      Glib::RefPtr<Gio::Settings> settings;

      auto children = base_settings_->list_children();
      if (std::find(children.begin(), children.end(), id) != children.end())
      {
        // If there exists a child schema with the given id we are going to use it
        // instead of the entry schema. This behaviour adds some flexibility and is
        // useful to define default entries for the list in the schema file.
        settings = base_settings_->get_child(id);
      }
      else
      {
        std::string entry_path = makeEntryPath(id);
        settings = Gio::Settings::create(entry_schema_id_, entry_path);
      }
      entries_settings_[id] = settings;
      return settings;
    }

  std::string makeEntryPath(const Identifier& id)
    { return base_settings_->property_path().get_value() + id + "/"; }

  void removeEntrySettings(const Identifier& id)
    { entries_settings_.erase(id); }

  static void resetRecursively(Glib::RefPtr<Gio::Settings> settings)
    {
      if (!settings)
        return;

      for (auto& child : settings->list_children())
        resetRecursively(settings->get_child(child));

      for (const auto& key: settings->list_keys())
        settings->reset(key);
    }
};

#endif//GMetronome_SettingsList_h
