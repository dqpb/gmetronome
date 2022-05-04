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

struct SettingsTreeNode
{
  Glib::RefPtr<Gio::Settings> settings;
  std::map<Glib::ustring, SettingsTreeNode> children;
};

// clients need to specialize this converter class template
template<typename ValueType>
struct SettingsListDelegate
{
  static ValueType load(const SettingsTreeNode& settings_tree);
  static void store(const SettingsTreeNode& settings_tree, const ValueType& value);
  static bool modified(const SettingsTreeNode& settings_tree);
};

template<typename ValueType>
class SettingsList : public Glib::Object {
public:
  using Identifier = Glib::ustring;
  using Delegate = SettingsListDelegate<ValueType>;

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

  ~SettingsList()
    {
      if (base_settings_)
        base_settings_->apply();

      std::for_each(entries_settings_.begin(), entries_settings_.end(),
                    [&] (auto& pair) { applyRecursively(pair.second); });

      g_settings_sync();
    }

  ValueType get(const Identifier& id)
    { return Delegate::load(getCachedEntrySettings(id)); }

  void update(const Identifier& id, const ValueType& value)
    { Delegate::store(getCachedEntrySettings(id), value); }

  void reset(const Identifier& id)
    { resetRecursively(getCachedEntrySettings(id)); }

  bool modified(const Identifier& id)
    { return Delegate::modified(getCachedEntrySettings(id)); }

  Identifier append(const ValueType& value)
    {
      gchar* uuid = g_uuid_string_random();
      Identifier id {uuid};
      g_free(uuid);

      const auto& settings_tree = createEntrySettings(id);
      Delegate::store(settings_tree, value);

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
      reset(id);
      removeEntrySettings(id);
    }

  bool select(const Identifier& id)
    {
      bool success = false;

      if (id.empty())
      {
        base_settings_->set_string(settings::kKeySettingsListSelectedEntry, "");
        success = true;
      }
      else if (auto l = list(); std::find(l.begin(), l.end(), id) != l.end())
      {
        base_settings_->set_string(settings::kKeySettingsListSelectedEntry, id);
        success = true;
      }
      else
      {
#ifndef NDEBUG
        std::cerr << "SettingsList: could not select entry '" << id << "' (not found)"
                  << std::endl;
#endif
        success = false;
      }
      return success;
    }

  Identifier selected()
    { return base_settings_->get_string(settings::kKeySettingsListSelectedEntry); }

  // returns a complete list of entries
  std::vector<Identifier> list(bool include_defaults = true)
    {
      std::vector<Identifier> list = entries();

      if (include_defaults)
      {
        // Check if the list contains the default entries (child schemas) and insert
        // them in front of the list if absent.
        // TODO: optimize to prevent unnecessary vector copies
        auto children = defaults();
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
          if (std::find(list.begin(), list.end(), *it) == list.end())
            list.insert(list.begin(), std::move(*it));
        }
      }
      else
      {
        // Remove default entries (if there are any)
        // TODO: optimize to prevent unnecessary vector copies
        for (const auto& child : defaults())
        {
          if (auto it = std::find(list.begin(), list.end(), child); it != list.end())
            list.erase(it);
        }
      }
      return list;
    }

  // returns a list as stored in the settings backend
  std::vector<Identifier> entries() const
    {
      // TODO: ckeck list for id uniqueness
      return base_settings_->get_string_array(settings::kKeySettingsListEntries);
    }

  // returns the list defaults
  std::vector<Identifier> defaults() const
    {
      auto defaults = base_settings_->list_children();
      auto defaults_swap_it = defaults.begin();

      // if defaults occur in the entry list, we synchronize the order
      for (const auto& entry : entries())
      {
        if (auto it = std::find(defaults.begin(), defaults.end(), entry); it != defaults.end())
          std::swap(*defaults_swap_it++, *it);
      }
      return defaults;
    }

  bool contains(const Identifier& id)
    {
      auto entry_list = entries();
      if (std::find(entry_list.begin(), entry_list.end(), id) != entry_list.end())
        return true;

      auto defaults_list = defaults();
      if (std::find(defaults_list.begin(), defaults_list.end(), id) != defaults_list.end())
        return true;

      return false;
    }

  const SettingsTreeNode& settings(const Identifier& id)
    { return getCachedEntrySettings(id); }

  Glib::RefPtr<Gio::Settings> settings()
    { return base_settings_; }

private:
  std::string entry_schema_id_;
  Glib::RefPtr<Gio::Settings> base_settings_;
  std::map<Identifier, SettingsTreeNode> entries_settings_;

  bool hasEntrySettings(const Identifier& id)
    {
      if (auto it = entries_settings_.find(id); it != entries_settings_.end())
        return true;
      else
        return false;
    }

  SettingsTreeNode& getCachedEntrySettings(const Identifier& id)
    {
      if (auto it = entries_settings_.find(id); it != entries_settings_.end())
        return it->second;
      else if (const auto& l = list(); std::find(l.begin(), l.end(), id) != l.end())
        return createEntrySettings(id);
      else
        throw GMetronomeError {"invalid settings list entry identifier"};
    }

  void buildSettingsTree(SettingsTreeNode& node)
    {
      if (node.settings)
      {
        auto children = node.settings->list_children();
        for (auto& child_name : children)
        {
          auto child_settings = node.settings->get_child(child_name);
          SettingsTreeNode child_node = {child_settings, {}};
          auto result = node.children.insert_or_assign(child_name, std::move(child_node));
          buildSettingsTree(result.first->second);
        }
      }
    }

  SettingsTreeNode& createEntrySettings(const Identifier& id)
    {
      Glib::RefPtr<Gio::Settings> settings;

      if (id.empty())
        throw GMetronomeError {"invalid settings list entry identifier"};

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
#ifndef NDEBUG
      if (!settings)
        std::cerr << "SettingsList: could not create Gio::Settings for entry "
                  << "'" << id << "'" << std::endl;
#endif
      SettingsTreeNode root = {settings, {}};
      buildSettingsTree(root);
      auto result =  entries_settings_.insert_or_assign(id, std::move(root));
      return result.first->second;
    }

  std::string makeEntryPath(const Identifier& id)
    { return base_settings_->property_path().get_value() + id + "/"; }

  void removeEntrySettings(const Identifier& id)
    {
      if (auto it = entries_settings_.find(id); it != entries_settings_.end())
      {
        applyRecursively(it->second);
        entries_settings_.erase(it);
      }
    }

  static void resetRecursively(const SettingsTreeNode& settings_tree)
    {
      std::for_each(settings_tree.children.begin(),
                    settings_tree.children.end(),
                    [] (const auto& pair) { resetRecursively(pair.second); });

      if (settings_tree.settings)
      {
        settings_tree.settings->delay();
        for (const auto& key: settings_tree.settings->list_keys())
        {
          settings_tree.settings->reset(key);
        }
        settings_tree.settings->apply();
      }
    }

  static void applyRecursively(const SettingsTreeNode& settings_tree)
    {
      std::for_each(settings_tree.children.begin(),
                    settings_tree.children.end(),
                    [] (const auto& pair) { applyRecursively(pair.second); });

      if (settings_tree.settings)
        settings_tree.settings->apply();
    }
};

#endif//GMetronome_SettingsList_h
