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

#ifndef GMetronome_SoundThemeListStoreXML_h
#define GMetronome_SoundThemeListStoreXML_h

#include "ListStoreXML.h"
#include "SoundTheme.h"

#include <glibmm/ustring.h>
#include <string>
#include <stack>

/**
 * @class SoundThemeParser
 */
class SoundThemeParser : public ListStoreXMLParser<SoundTheme, SoundTheme::Identifier> {
public:
  SoundThemeParser() = default;
  SoundThemeParser(SoundThemeParser&& other) = default;
  SoundThemeParser& operator=(SoundThemeParser&& other) = default;
  ~SoundThemeParser() override = default;

  EntryMap moveMap() override
    { return std::move(t_map_); }
  OrderVector moveOrder() override
    { return std::move(t_order_); }

private:
  EntryMap t_map_;
  OrderVector t_order_;
  std::stack<Glib::ustring> current_block_;
  SoundTheme* current_theme_{nullptr};
  audio::SoundParameters* current_params_{nullptr};

private:
  void on_start_element (Glib::Markup::ParseContext& context,
                         const Glib::ustring& element_name,
                         const AttributeMap& attributes) override;

  void on_end_element (Glib::Markup::ParseContext& context,
                       const Glib::ustring& element_name) override;

  void on_text (Glib::Markup::ParseContext& context,
                const Glib::ustring& text) override;
};

/**
 * @class SoundThemeWriter
 */
class SoundThemeWriter : public ListStoreXMLWriter<SoundTheme, SoundTheme::Identifier> {
public:
  const std::string& topLevelElementName() const override
    { return kTopLevelElementName; }

  void writeEntry(Glib::RefPtr<Gio::FileOutputStream> ostream,
                  const SoundTheme& theme,
                  const Identifier& id) override;
private:
  inline static const std::string kTopLevelElementName {"sound-themes"};
};

/**
 * @class SoundThemeListStoreXML
 */
struct SoundThemeListStoreXML
  : public ListStoreXML<SoundTheme,
                        SoundTheme::Identifier,
                        SoundTheme::Header,
                        SoundThemeParser,
                        SoundThemeWriter>
{
  SoundThemeListStoreXML(std::string path, std::string import_path = "")
    : ListStoreXML(path, import_path)
  { /* nothing */ }
};

#endif//GMetronome_SoundThemeListStoreXML_h
