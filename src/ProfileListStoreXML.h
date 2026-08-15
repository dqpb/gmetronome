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

#ifndef GMetronome_ProfileListStoreXML_h
#define GMetronome_ProfileListStoreXML_h

#include "ListStoreXML.h"
#include "Profile.h"
#include "Meter.h"

#include <glibmm/ustring.h>
#include <string>
#include <stack>

/**
 * @class ProfileParser
 */
class ProfileParser : public ListStoreXMLParser<Profile, Profile::Identifier> {
public:
  ProfileParser() = default;
  ProfileParser(ProfileParser&& other) = default;
  ProfileParser& operator=(ProfileParser&& other) = default;
  ~ProfileParser() override = default;

  EntryMap moveMap() override
    { return std::move(pmap_); }
  OrderVector moveOrder() override
    { return std::move(porder_); }

private:
  EntryMap pmap_;
  OrderVector porder_;
  Profile* current_profile_{nullptr};
  Meter* current_meter_{nullptr};
  int current_meter_division_{0};
  int current_meter_beats_{0};
  AccentPattern current_meter_accents_;
  std::stack<Glib::ustring> current_block_;

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
 * @class ProfileWriter
 */
class ProfileWriter : public ListStoreXMLWriter<Profile, Profile::Identifier> {
public:
  const std::string& topLevelElementName() const override
    { return kTopLevelElementName; }

  void writeEntry(Glib::RefPtr<Gio::FileOutputStream> ostream,
                  const Profile& profile,
                  const Identifier& id) override;
private:
  inline static const std::string kTopLevelElementName {"profiles"};
};

/**
 * @class ProfileListStoreXML
 */
struct ProfileListStoreXML
  : public ListStoreXML<Profile,
                        Profile::Identifier,
                        Profile::Header,
                        ProfileParser,
                        ProfileWriter>
{
  ProfileListStoreXML(std::string path, std::string import_path = "")
    : ListStoreXML(path, import_path)
  { /* nothing */ }
};

#endif//GMetronome_ProfileListStoreXML_h
