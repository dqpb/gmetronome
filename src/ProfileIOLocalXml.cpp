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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ProfileIOLocalXml.h"
#include "Error.h"
#include <iterator>
#include <charconv>
#include <iostream>
#include <cassert>
#include <stack>
#include <array>

#ifndef HAVE_CPP_LIB_TO_CHARS
# include <sstream>
#endif

ProfileIOLocalXml::ProfileIOLocalXml(Glib::RefPtr<Gio::File> file)
  : file_{file},
    pmap_{},
    porder_{},
    pending_import_ {true},
    import_error_ {false},
    pending_export_ {false},
    export_error_ {false}
{}

ProfileIOLocalXml::~ProfileIOLocalXml()
{
  if (pending_export_)
  {
    try { exportProfiles(); }
    catch (const GMetronomeError& e) {
#ifndef NDEBUG
      std::cerr << "ProfileIOLocalXml: failed to save profiles "
                << "('" << e.what() << "')" << std::endl;
#endif
    }
    catch (...) {
#ifndef NDEBUG
      std::cerr << "ProfileIOLocalXml: failed to save profiles" << std::endl;
#endif
    }
  }
}

std::vector<Profile::Primer> ProfileIOLocalXml::list()
{
  if (pending_import_ && !import_error_)
    importProfiles();

  std::vector<Profile::Primer> primers;
  primers.reserve(porder_.size());

  std::transform(porder_.begin(), porder_.end(), std::back_inserter(primers),
                 [this] (const auto& id) -> Profile::Primer {
                   return {id, pmap_[id].header};
                 });

  return primers;
}

Profile ProfileIOLocalXml::load(Profile::Identifier id)
{
  if (pending_import_ && !import_error_)
    importProfiles();

  try {
    return pmap_.at(id);
  }
  catch (...) {
    throw GMetronomeError {"profile with id '" + id + "' does not exist"};
  }
}

void ProfileIOLocalXml::store(Profile::Identifier id, const Profile& profile)
{
  if (pending_import_ && !import_error_)
    importProfiles();

  if (auto it = pmap_.find(id); it != pmap_.end())
  {
    it->second = profile;;
  }
  else
  {
    pmap_[id] = profile;
    porder_.push_back(id);
  }

  pending_export_ = true;
}

void ProfileIOLocalXml::reorder(const std::vector<Profile::Identifier>& order)
{
  if (pending_import_ && !import_error_)
    importProfiles();

  std::vector<std::size_t> a;
  a.reserve(order.size());

  for (const auto& id : order)
  {
    if (auto it = std::find(porder_.begin(), porder_.end(), id);
        it != porder_.end())
    {
      a.push_back(std::distance(porder_.begin(), it));
    }
  }

  auto b = a; // make a copy
  std::sort(b.begin(), b.end());

  if (auto it = std::adjacent_find(b.begin(), b.end()); it == b.end())
  {
    decltype(porder_) new_porder;
    new_porder.reserve(porder_.size());

    std::transform(a.begin(), a.end(), std::back_inserter(new_porder),
                   [this] (const auto& idx) { return porder_[idx]; });

    std::swap(porder_, new_porder);
  }

  pending_export_ = true;
}

void ProfileIOLocalXml::remove(Profile::Identifier id)
{
  if (pending_import_ && !import_error_)
    importProfiles();

  if (auto it = std::find(porder_.begin(), porder_.end(), id);
      it != porder_.end())
  {
    porder_.erase(it);
  }
  pmap_.erase(id);
  pending_export_ = true;
}

void ProfileIOLocalXml::flush()
{
  if (pending_export_)
  {
    if (pending_import_ && !import_error_)
      importProfiles();

    exportProfiles();
  }
}

Glib::RefPtr<Gio::File> ProfileIOLocalXml::defaultFile()
{
  auto path = Glib::build_filename( Glib::get_user_data_dir(),
                                    PACKAGE,
                                    "profiles.xml" );

  return Gio::File::create_for_path( path );
}


namespace {

  constexpr int kConvBufSize = 50;

  template<class T>
  std::string numberToString(const T& value)
  {
#ifdef HAVE_CPP_LIB_TO_CHARS
    std::array<char,kConvBufSize> str;
    if(auto [p, ec] = std::to_chars(str.data(), str.data() + str.size(), value);
       ec == std::errc())
      return std::string(str.data(), p - str.data());
    else
      throw std::runtime_error {"failed to convert number to string"};
#else
    std::stringstream sstr;
    sstr.imbue(std::locale::classic());
    sstr << value;

    std::string s;
    sstr >> s;

    if (sstr.fail())
      throw std::runtime_error {"failed to convert number to string"};

    return s;
#endif
  }

  template<class T>
  T stringToNumber(const std::string& str)
  {
#ifdef HAVE_CPP_LIB_TO_CHARS
    T value;
    if(auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
       ec == std::errc())
      return value;
    else
      throw std::runtime_error {"failed to convert string to number"};
#else
    std::stringstream sstr;
    sstr.imbue(std::locale::classic());
    sstr << str;

    T value;
    sstr >> value;

    if (sstr.fail())
      throw std::runtime_error {"failed to convert string to number"};

    return value;
#endif
  }

  std::string doubleToString(double value)
  { return numberToString(std::round(value * 100.0) / 100.0); }

  double stringToDouble(const std::string& str)
  { return std::round(stringToNumber<double>(str) * 100.0) / 100.0; }

  std::string intToString(int value)
  { return numberToString(value); }

  int stringToInt(const std::string& str)
  { return stringToNumber<int>(str); }

  std::string boolToString(bool value)
  {
    return value ? "true" : "false";
  }

  bool stringToBool(const Glib::ustring& text)
  {
    auto text_lowercase = text.lowercase();
    if (text_lowercase == "true")
      return true;
    else if (text_lowercase == "false")
      return false;
    else if (std::stoi(text) == 0)
      return false;
    else
      return true;
  }

  using ProfileMap = ProfileIOLocalXml::ProfileMap;

  class MarkupParser : public Glib::Markup::Parser {
  public:
    MarkupParser()
      : current_profile_ {nullptr},
        current_meter_ {nullptr},
        current_meter_division_ {0},
        current_meter_beats_ {0}
      {}

    ProfileMap move_pmap() const
      { return std::move(pmap_); }

    std::vector<Profile::Identifier> move_porder() const
      { return std::move(porder_); }

  protected:
    void on_start_element (Glib::Markup::ParseContext& context,
                           const Glib::ustring& element_name,
                           const AttributeMap& attributes) override
      {
        try {
          auto element_name_lowercase = element_name.lowercase();
          if (element_name_lowercase == "header"
              || element_name_lowercase == "content"
              || element_name_lowercase == "sound-theme"
              || element_name_lowercase == "trainer-section"
              || element_name_lowercase == "meter-section")
          {
            current_block_.push(element_name_lowercase);
          }
          else if (element_name_lowercase == "profile")
          {
            auto attrib_it = std::find_if(attributes.begin(), attributes.end(),
                                          [] (auto& pair) {
                                            return pair.first.lowercase() == "id";
                                          });

            if (attrib_it != attributes.end())
            {
              if (auto pmap_it = pmap_.find(attrib_it->second); pmap_it != pmap_.end())
              {
                current_profile_ = &pmap_it->second;
              }
              else
              {
                current_profile_ = &pmap_[attrib_it->second];
                porder_.push_back(attrib_it->second);
              }
            }
            else
            {
              current_profile_ = nullptr;
            }
          }
          else if (element_name_lowercase == "meter")
          {
            current_block_.push(element_name_lowercase);

            auto it = std::find_if(attributes.begin(), attributes.end(),
                                   [] (auto& pair) {
                                     return pair.first.lowercase() == "id";
                                   });

            if (current_profile_ && it != attributes.end())
            {
              if (it->second == "meter-simple-2")
                current_meter_ = &current_profile_->content.meter_simple_2;
              else if (it->second == "meter-simple-3")
                current_meter_ = &current_profile_->content.meter_simple_3;
              else if (it->second == "meter-simple-4")
                current_meter_ = &current_profile_->content.meter_simple_4;
              else if (it->second == "meter-compound-2")
                current_meter_ = &current_profile_->content.meter_compound_2;
              else if (it->second == "meter-compound-3")
                current_meter_ = &current_profile_->content.meter_compound_3;
              else if (it->second == "meter-compound-4")
                current_meter_ = &current_profile_->content.meter_compound_4;
              else if (it->second == "meter-custom")
                current_meter_ = &current_profile_->content.meter_custom;
            }
            else
            {
              current_meter_ = nullptr;
            }

            current_meter_division_ = kNoDivision;
            current_meter_beats_ = kSingleMeter;
            current_meter_accents_.clear();
          }
          else if (element_name_lowercase == "accent")
          {
            auto it = std::find_if(attributes.begin(), attributes.end(),
                                   [] (auto& pair) {
                                     return pair.first.lowercase() == "level";
                                   });
            if (it != attributes.end())
              current_meter_accents_.push_back( static_cast<Accent>(stringToInt(it->second)) );
          }
        }
        catch(const std::exception& error)
        {
          std::string msg = makeErrorMessage(context, error.what());
          throw Glib::MarkupError {Glib::MarkupError::Code::INVALID_CONTENT, msg};
        }
      }

    void on_end_element (Glib::Markup::ParseContext& context,
                         const Glib::ustring& element_name) override
      {
        auto element_name_lowercase = element_name.lowercase();
        if (element_name_lowercase == "header"
            || element_name_lowercase == "content"
            || element_name_lowercase == "sound-theme"
            || element_name_lowercase == "trainer-section"
            || element_name_lowercase == "meter-section")
        {
          current_block_.pop();
        }
        else if (element_name_lowercase == "profile")
        {
          current_profile_ = nullptr;
        }
        else if (element_name_lowercase == "meter")
        {
          if (current_meter_ != nullptr) {

            (*current_meter_) = Meter
              {
                current_meter_division_,
                current_meter_beats_,
                current_meter_accents_
              };
          }
          current_block_.pop();
          current_meter_ = nullptr;
        }
      }

    void on_text (Glib::Markup::ParseContext& context,
                  const Glib::ustring& text) override
      {
        if (current_profile_ && !current_block_.empty())
        {
          auto element_name_lowercase = context.get_element().lowercase();

          try {
            if (current_block_.top() == "header")
            {
              if (element_name_lowercase == "title")
                current_profile_->header.title = text;
              else if (element_name_lowercase == "description")
                current_profile_->header.description = text;
            }
            else if (current_block_.top() == "content")
            {
              if (element_name_lowercase == "tempo")
                current_profile_->content.tempo = stringToDouble(text);
            }
            else if (current_block_.top() == "sound-theme")
            {
              if (element_name_lowercase == "ref-id")
                current_profile_->content.sound_theme_id = text;
            }
            else if (current_block_.top() == "meter-section")
            {
              if (element_name_lowercase == "enabled")
                current_profile_->content.meter_enabled = stringToBool(text);
              else if (element_name_lowercase == "meter-select")
                current_profile_->content.meter_select = text;
            }
            else if (current_block_.top() == "meter" && current_meter_)
            {
              if (element_name_lowercase == "division")
                current_meter_division_ = stringToInt(text);
              else if (element_name_lowercase == "beats")
                current_meter_beats_ = stringToInt(text);
            }
            else if (current_block_.top() == "trainer-section")
            {
              if (element_name_lowercase == "enabled")
                current_profile_->content.trainer_enabled = stringToBool(text);
              else if (element_name_lowercase == "mode")
                current_profile_->content.trainer_mode =
                  static_cast<Profile::TrainerMode>(stringToInt(text));
              else if (element_name_lowercase == "target")
                current_profile_->content.trainer_target = stringToDouble(text);
              else if (element_name_lowercase == "accel")
                current_profile_->content.trainer_accel = stringToDouble(text);
              else if (element_name_lowercase == "step")
                current_profile_->content.trainer_step = stringToDouble(text);
              else if (element_name_lowercase == "hold")
                current_profile_->content.trainer_hold = stringToInt(text);
            }
          }
          catch(const std::exception& error)
          {
            std::string msg = makeErrorMessage(context, error.what());
            throw Glib::MarkupError {Glib::MarkupError::Code::INVALID_CONTENT, msg};
          }
        }
      }

    void on_passthrough (Glib::Markup::ParseContext& context,
                         const Glib::ustring& passthrough_text) override
      {}

    void on_error (Glib::Markup::ParseContext& context,
                   const Glib::MarkupError& error) override
      {}

    std::string makeErrorMessage(Glib::Markup::ParseContext& context,
                                 const std::string& msg)
      {
        return "error on line " + std::to_string(context.get_line_number())
          + ", char " + std::to_string(context.get_char_number())
          + ": " + msg;
      }

  private:
    ProfileMap pmap_;
    std::vector<Profile::Identifier> porder_;
    Profile* current_profile_;
    Meter* current_meter_;
    int current_meter_division_;
    int current_meter_beats_;
    AccentPattern current_meter_accents_;
    std::stack<Glib::ustring> current_block_;
  };

}//unnamed namespace

void ProfileIOLocalXml::importProfiles()
{
  MarkupParser parser;
  Glib::Markup::ParseContext context(parser);

  std::array<char, 4096> buffer;

  try {
    auto input_stream = file_->read();
    for (auto bytes_read = input_stream->read(buffer.data(), buffer.size());
         bytes_read > 0;
         bytes_read = input_stream->read(buffer.data(), buffer.size()))
    {
      context.parse(buffer.data(), buffer.data() + bytes_read);
    }
    context.end_parse();
    pmap_ = parser.move_pmap();
    porder_ = parser.move_porder();
  }
  catch(const Gio::Error& e)
  {
    switch (e.code()) {
    case Gio::Error::NOT_FOUND:
      // ignore (file might not have been created yet)
      break;
    default:
      import_error_ = true;
      throw GMetronomeError { e.what() };
      break;
    }
  }
  catch(const Glib::MarkupError& e)
  {
    import_error_ = true;
    throw GMetronomeError { e.what() };
  }
  catch(...)
  {
    import_error_ = true;
    throw;
  }
  pending_import_ = false;
}

namespace {

  Glib::RefPtr<Gio::FileOutputStream> createOutputStream( Glib::RefPtr<Gio::File> file )
  {
    Glib::RefPtr<Gio::FileOutputStream> ostream;
    static const Gio::FileCreateFlags flags = Gio::FILE_CREATE_PRIVATE;

    try {
      ostream = file->replace( std::string(), false, flags );
    }
    catch (const Gio::Error& replace_error)
    {
      if (replace_error.code() == Gio::Error::NOT_FOUND)
      {
        try {
          auto parent_dir = file->get_parent();
          parent_dir->make_directory_with_parents();
          ostream = file->create_file( flags );
        }
        catch (const Gio::Error& mkdir_error)
        {
          throw GMetronomeError { mkdir_error.what() };
        }
      }
      else throw GMetronomeError { replace_error.what() };
    }
    return ostream;
  }

  void writeProfileHeader(Glib::RefPtr<Gio::FileOutputStream> ostream, const Profile& profile)
  {
    ostream->write("    <header>\n");
    ostream->write("      <title>");
    auto text = Glib::Markup::escape_text(profile.header.title);
    ostream->write(text);
    ostream->write("</title>\n");
    ostream->write("      <description>");
    text = Glib::Markup::escape_text(profile.header.description);
    ostream->write(text);
    ostream->write("</description>\n");
    ostream->write("    </header>\n");
  }

  void writeProfileContent_Meter(Glib::RefPtr<Gio::FileOutputStream> ostream,
                                 const Meter& meter,
                                 const std::string& meter_id)
  {
    ostream->write("          <meter id=\"");
    ostream->write(Glib::Markup::escape_text(meter_id));
    ostream->write("\">\n");
    ostream->write("            <division>");
    ostream->write(intToString(meter.division()));
    ostream->write("</division>\n");
    ostream->write("            <beats>");
    ostream->write(intToString(meter.beats()));
    ostream->write("</beats>\n");
    ostream->write("            <accent-pattern>\n");
    const AccentPattern& accents = meter.accents();
    for ( const Accent& a : accents )
    {
      ostream->write("              <accent level=\"");
      int i = static_cast<int>(a);
      ostream->write(intToString(i));
      ostream->write("\"/>\n");
    }
    ostream->write("            </accent-pattern>\n");
    ostream->write("          </meter>\n");
  }

  void writeProfileContent(Glib::RefPtr<Gio::FileOutputStream> ostream, const Profile& profile)
  {
    auto& content = profile.content;

    ostream->write("    <content>\n");
    ostream->write("      <sound-theme>\n");
    ostream->write("        <ref-id>");
    ostream->write(Glib::Markup::escape_text(content.sound_theme_id));
    ostream->write("</ref-id>\n");
    ostream->write("      </sound-theme>\n");
    ostream->write("      <tempo>");
    ostream->write(doubleToString(content.tempo));
    ostream->write("</tempo>\n");
    ostream->write("      <meter-section>\n");
    ostream->write("        <enabled>");
    ostream->write(boolToString(content.meter_enabled));
    ostream->write("</enabled>\n");

    ostream->write("        <meter-select>");
    ostream->write(Glib::Markup::escape_text(content.meter_select));
    ostream->write("</meter-select>\n");

    ostream->write("        <meter-list>\n");
    writeProfileContent_Meter(ostream, content.meter_simple_2, "meter-simple-2");
    writeProfileContent_Meter(ostream, content.meter_simple_3, "meter-simple-3");
    writeProfileContent_Meter(ostream, content.meter_simple_4, "meter-simple-4");
    writeProfileContent_Meter(ostream, content.meter_compound_2, "meter-compound-2");
    writeProfileContent_Meter(ostream, content.meter_compound_3, "meter-compound-3");
    writeProfileContent_Meter(ostream, content.meter_compound_4, "meter-compound-4");
    writeProfileContent_Meter(ostream, content.meter_custom, "meter-custom");
    ostream->write("        </meter-list>\n");

    ostream->write("      </meter-section>\n");
    ostream->write("      <trainer-section>\n");
    ostream->write("        <enabled>");
    ostream->write(boolToString(content.trainer_enabled));
    ostream->write("</enabled>\n");
    ostream->write("        <mode>");
    ostream->write(intToString(static_cast<int>(content.trainer_mode)));
    ostream->write("</mode>\n");
    ostream->write("        <target>");
    ostream->write(doubleToString(content.trainer_target));
    ostream->write("</target>\n");
    ostream->write("        <accel>");
    ostream->write(doubleToString(content.trainer_accel));
    ostream->write("</accel>\n");
    ostream->write("        <step>");
    ostream->write(doubleToString(content.trainer_step));
    ostream->write("</step>\n");
    ostream->write("        <hold>");
    ostream->write(intToString(content.trainer_hold));
    ostream->write("</hold>\n");
    ostream->write("      </trainer-section>\n");
    ostream->write("    </content>\n");
  }

  void writeProfile(Glib::RefPtr<Gio::FileOutputStream> ostream,
                    const Profile& profile, std::string id)
  {
    ostream->write("  <profile id=\"");
    ostream->write(id);
    ostream->write("\">\n");

    writeProfileHeader(ostream, profile);
    writeProfileContent(ostream, profile);

    ostream->write("  </profile>\n");
  }

}//unnamed namespace

void ProfileIOLocalXml::exportProfiles()
{
  Glib::RefPtr<Gio::FileOutputStream> ostream;
  try {
    ostream = createOutputStream(file_);
  }
  catch (...)
  {
    export_error_ = true;
    throw;
  }

  assert(ostream);

  ostream->write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  ostream->write("<" PACKAGE "-profiles version=\"" PACKAGE_VERSION "\">\n");
  for (const auto& id : porder_)
  {
    writeProfile(ostream, pmap_[id], id);
  }
  ostream->write("</" PACKAGE "-profiles>\n");
  ostream->flush();
  ostream->close();
}
