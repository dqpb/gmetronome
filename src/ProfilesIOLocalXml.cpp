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

#include "ProfilesIOLocalXml.h"
#include "config.h"
#include <iterator>
#include <iostream>
#include <stack>

ProfilesIOLocalXml::ProfilesIOLocalXml(Glib::RefPtr<Gio::File> file)
  : file_(file)
{
  importProfiles();
}

ProfilesIOLocalXml::~ProfilesIOLocalXml()
{
  exportProfiles();
}

std::vector<Profile::Primer> ProfilesIOLocalXml::list() 
{
  std::vector<Profile::Primer> vec;
  
  for (const auto& id : porder_)
    vec.push_back({id,pmap_[id].header});

  return vec;
}

Profile ProfilesIOLocalXml::load(Profile::Identifier id) 
{
  try {
    return pmap_.at(id);
  }
  catch (...) {
    throw;
  }
}

void ProfilesIOLocalXml::store(Profile::Identifier id, const Profile& profile) 
{
  if (auto it = pmap_.find(id); it != pmap_.end())
  {
    it->second = profile;;
  }
  else
  {
    pmap_[id] = profile;
    porder_.push_back(id);
  }
}

void ProfilesIOLocalXml::reorder(const std::vector<Profile::Identifier>& order)
{
  std::vector<std::size_t> a;
  a.reserve(order.size());

  for (const auto& id : order)
  {
    if (auto it = std::find(porder_.begin(), porder_.end(), id); it != porder_.end())
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
    
    for (const auto& idx : a)
      new_porder.push_back(porder_[idx]);
    
    std::swap(porder_, new_porder);
  }
}

void ProfilesIOLocalXml::remove(Profile::Identifier id)
{
  if (auto it = std::find(porder_.begin(), porder_.end(), id); it != porder_.end())
  {
    porder_.erase(it);
  }

  pmap_.erase(id);
}

void ProfilesIOLocalXml::flush() 
{
  exportProfiles();
}

Glib::RefPtr<Gio::File> ProfilesIOLocalXml::defaultFile()
{
  auto path = Glib::build_filename( Glib::get_user_data_dir(),
				    PACKAGE,
				    "profiles.xml" );
  
  return Gio::File::create_for_path( path );
}


namespace {

  using ProfileMap = ProfilesIOLocalXml::ProfileMap;
  
  class MarkupParser : public Glib::Markup::Parser {
  public:
    MarkupParser() : current_profile_(nullptr), current_meter_(nullptr) {}
    ~MarkupParser() override {}

    ProfileMap move_pmap() const
      { return std::move(pmap_); }

    std::vector<Profile::Identifier> move_porder() const
      { return std::move(porder_); }

  protected:

    bool stobool(const Glib::ustring& text)
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

    
    void on_start_element (Glib::Markup::ParseContext& context,
                           const Glib::ustring& element_name,
                           const AttributeMap& attributes) override
      {
        auto element_name_lowercase = element_name.lowercase();
        if (element_name_lowercase == "header"
            || element_name_lowercase == "content"
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
            if (it->second == "meter-1-simple")
              current_meter_ = &current_profile_->content.meter_1_simple;
            else if (it->second == "meter-2-simple")
              current_meter_ = &current_profile_->content.meter_2_simple;
            else if (it->second == "meter-3-simple")
              current_meter_ = &current_profile_->content.meter_3_simple;
            else if (it->second == "meter-4-simple")
              current_meter_ = &current_profile_->content.meter_4_simple;
            else if (it->second == "meter-1-compound")
              current_meter_ = &current_profile_->content.meter_1_compound;
            else if (it->second == "meter-2-compound")
              current_meter_ = &current_profile_->content.meter_2_compound;
            else if (it->second == "meter-3-compound")
              current_meter_ = &current_profile_->content.meter_3_compound;
            else if (it->second == "meter-4-compound")
              current_meter_ = &current_profile_->content.meter_4_compound;
            else if (it->second == "meter-custom")
              current_meter_ = &current_profile_->content.meter_custom;
          }
          else
          {
            current_meter_ = nullptr;
          }

          current_meter_beats_ = kSingleMeter;
          current_meter_division_ = kNoDivision;
          current_meter_accents_.clear();
        }
        else if (element_name_lowercase == "accent")
        {
          auto it = std::find_if(attributes.begin(), attributes.end(),
                                 [] (auto& pair) {
                                   return pair.first.lowercase() == "level";
                                 });
          if (it != attributes.end())
            current_meter_accents_.push_back( static_cast<Accent>(std::stoi(it->second)) );
        }

      }
    
    void on_end_element (Glib::Markup::ParseContext& context,
                         const Glib::ustring& element_name) override
      {
        auto element_name_lowercase = element_name.lowercase();
        if (element_name_lowercase == "header"
            || element_name_lowercase == "content"
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
            
            (*current_meter_) =
              {
                current_meter_beats_,
                current_meter_division_,
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
              current_profile_->content.tempo = std::stoi(text);
          }
          else if (current_block_.top() == "meter-section")
          {
            if (element_name_lowercase == "enabled")
              current_profile_->content.meter_enabled = stobool(text);
            else if (element_name_lowercase == "meter-select")
              current_profile_->content.meter_select = text;
          }
          else if (current_block_.top() == "meter" && current_meter_)
          {
            if (element_name_lowercase == "beats")
              current_meter_beats_ = std::stoi(text);
            else if (element_name_lowercase == "division")
              current_meter_division_ = std::stoi(text);
          }
          else if (current_block_.top() == "trainer-section")
          {
            if (element_name_lowercase == "enabled")
              current_profile_->content.trainer_enabled = stobool(text);
            else if (element_name_lowercase == "start")
              current_profile_->content.trainer_start = std::stoi(text);
            else if (element_name_lowercase == "target")
              current_profile_->content.trainer_target = std::stoi(text);
            else if (element_name_lowercase == "accel")
              current_profile_->content.trainer_accel = std::stoi(text);
          }
        }
      }
    
    void on_passthrough (Glib::Markup::ParseContext& context,
                         const Glib::ustring& passthrough_text) override
      {
        //not implemented yet
      }

    void on_error (Glib::Markup::ParseContext& context,
                   const Glib::MarkupError& error) override
      {
        //not implemented yet
      }

  private:
    ProfileMap pmap_;
    std::vector<Profile::Identifier> porder_;
    Profile* current_profile_;
    Meter* current_meter_;
    int current_meter_beats_;
    int current_meter_division_;
    AccentPattern current_meter_accents_;
    std::stack<Glib::ustring> current_block_;
  };

  
  Glib::RefPtr<Gio::FileOutputStream> createOutputStream( Glib::RefPtr<Gio::File> file )
  {
    static const Gio::FileCreateFlags flags = Gio::FILE_CREATE_PRIVATE;
    
    try {
      return file->replace( std::string(), false, flags );
    }
    catch (...) {}
  
    try {
      auto parent_dir = file->get_parent();
      parent_dir->make_directory_with_parents();
    }
    catch (...) {}

    try {
      return file->create_file( flags );
    }
    catch (...) {}
    
    return Glib::RefPtr<Gio::FileOutputStream>();
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
    ostream->write("            <beats>");
    ostream->write(std::to_string(meter.beats()));
    ostream->write("</beats>\n");
    ostream->write("            <division>");
    ostream->write(std::to_string(meter.division()));
    ostream->write("</division>\n");
    ostream->write("            <accent-pattern>\n");
    const AccentPattern& accents = meter.accents();
    for ( const Accent& a : accents )
    {
      ostream->write("              <accent level=\"");
      int i = static_cast<int>(a);
      ostream->write(std::to_string(i));
      ostream->write("\"/>\n");
    }
    ostream->write("            </accent-pattern>\n");
    ostream->write("          </meter>\n");    
  }

  void writeProfileContent(Glib::RefPtr<Gio::FileOutputStream> ostream, const Profile& profile)
  {
    auto& content = profile.content;
    
    ostream->write("    <content>\n");
    ostream->write("      <tempo>");
    ostream->write(std::to_string(content.tempo));
    ostream->write("</tempo>\n");
    ostream->write("      <meter-section>\n");
    ostream->write("        <enabled>");
    ostream->write(std::to_string(content.meter_enabled));
    ostream->write("</enabled>\n");
    
    ostream->write("        <meter-select>");
    ostream->write(Glib::Markup::escape_text(content.meter_select));
    ostream->write("</meter-select>\n");
    
    ostream->write("        <meter-list>\n");
    writeProfileContent_Meter(ostream, content.meter_1_simple, "meter-1-simple");
    writeProfileContent_Meter(ostream, content.meter_2_simple, "meter-2-simple");
    writeProfileContent_Meter(ostream, content.meter_3_simple, "meter-3-simple");
    writeProfileContent_Meter(ostream, content.meter_4_simple, "meter-4-simple");
    writeProfileContent_Meter(ostream, content.meter_1_compound, "meter-1-compound");
    writeProfileContent_Meter(ostream, content.meter_2_compound, "meter-2-compound");
    writeProfileContent_Meter(ostream, content.meter_3_compound, "meter-3-compound");
    writeProfileContent_Meter(ostream, content.meter_4_compound, "meter-4-compound");
    writeProfileContent_Meter(ostream, content.meter_custom, "meter-custom");
    ostream->write("        </meter-list>\n");
    
    ostream->write("      </meter-section>\n");
    ostream->write("      <trainer-section>\n");
    ostream->write("        <enabled>");
    ostream->write(std::to_string(content.trainer_enabled));
    ostream->write("</enabled>\n");
    ostream->write("        <start>");
    ostream->write(std::to_string(content.trainer_start));
    ostream->write("</start>\n");
    ostream->write("        <target>");
    ostream->write(std::to_string(content.trainer_target));
    ostream->write("</target>\n");
    ostream->write("        <accel>");
    ostream->write(std::to_string(content.trainer_accel));
    ostream->write("</accel>\n");
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


void ProfilesIOLocalXml::importProfiles()
{
  try {
    std::string file_contents = Glib::file_get_contents(file_->get_path());

    MarkupParser parser;
    Glib::Markup::ParseContext context(parser);
  
    context.parse(file_contents);
    context.end_parse();
    
    pmap_ = parser.move_pmap();
    porder_ = parser.move_porder();
  }
  catch(...) {}
}

void ProfilesIOLocalXml::exportProfiles()
{
  auto ostream = createOutputStream(file_);
  
  if (ostream) {
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
}

