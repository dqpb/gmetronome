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

#include "SoundThemeListStoreXML.h"
#include "Error.h"

#include <iterator>
#include <charconv>
#include <iostream>
#include <cassert>
#include <stack>
#include <array>
#include <string_view>
#include <utility>

#ifndef HAVE_CPP_LIB_TO_CHARS
# include <sstream>
#endif

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

#define RAMP_SHAPE_LIST                                         \
  X(audio::EnvelopeRampShape::kLinear,       "linear")          \
  X(audio::EnvelopeRampShape::kCubic,        "cubic")           \
  X(audio::EnvelopeRampShape::kCubicFlipped, "cubic-flipped")

  std::string rampShapeToString(audio::EnvelopeRampShape shape)
  {
    switch (shape) {
#define X(entry, entry_str) case entry: return entry_str;
      RAMP_SHAPE_LIST
#undef X
    default: return "";
    }
  }

  audio::EnvelopeRampShape stringToRampShape(const Glib::ustring& str)
  {
#define X(entry, entry_str) if (str.lowercase() == entry_str) { return entry; }
    RAMP_SHAPE_LIST
#undef X
      return audio::EnvelopeRampShape::kLinear;
  }

#define HOLD_SHAPE_LIST                                 \
  X(audio::EnvelopeHoldShape::kKeep,    "keep")         \
  X(audio::EnvelopeHoldShape::kQuartic, "quartic")

  std::string holdShapeToString(audio::EnvelopeHoldShape shape)
  {
    switch (shape) {
#define X(entry, entry_str) case entry: return entry_str;
      HOLD_SHAPE_LIST
#undef X
    default: return "";
    }
  }

  audio::EnvelopeHoldShape stringToHoldShape(const Glib::ustring& str)
  {
#define X(entry, entry_str) if (str.lowercase() == entry_str) { return entry; }
    HOLD_SHAPE_LIST
#undef X
      return audio::EnvelopeHoldShape::kKeep;
  }

  // Split sound key of form "<category>/<id>" to a pair {<category>, <id>}
  std::pair<std::string_view, std::string_view> splitSoundKey(std::string_view key)
  {
    const auto pos = key.find('/');

    if (pos == std::string_view::npos)
      return { key.substr(0, 0), key.substr(0) };
    else
      return { key.substr(0, pos), key.substr(pos + 1) };
  }

  std::string makeErrorMessage(Glib::Markup::ParseContext& context,
                               const std::string& msg)
  {
    return "error on line " + std::to_string(context.get_line_number())
      + ", char " + std::to_string(context.get_char_number())
      + ": " + msg;
  }
}//unnamed namespace

void SoundThemeParser::on_start_element (Glib::Markup::ParseContext& context,
                                         const Glib::ustring& element_name,
                                         const AttributeMap& attributes)
{
  try {
    auto element_name_lc = element_name.lowercase();

    if (element_name_lc == "header"
        || element_name_lc == "content"
        || element_name_lc == "sounds"
        || element_name_lc == "tone"
        || element_name_lc == "noise")
    {
      current_block_.push(element_name_lc);
    }
    else if (element_name_lc == "sound-theme")
    {
      current_block_.push(element_name_lc);

      auto id_it = std::find_if(attributes.begin(), attributes.end(),
                                [] (auto& pair) { return pair.first.lowercase() == "id"; });

      if (id_it != attributes.end())
      {
        if (auto t_map_it = t_map_.find(id_it->second); t_map_it != t_map_.end())
        {
          current_theme_ = &t_map_it->second;
        }
        else
        {
          current_theme_ = &t_map_[id_it->second];
          t_order_.push_back(id_it->second);
        }
      }
      else
        current_theme_ = nullptr;
    }
    else if (element_name_lc == "sound")
    {
      current_block_.push(element_name_lc);

      auto key_it = std::find_if(attributes.begin(), attributes.end(),
                                 [] (auto& pair) { return pair.first.lowercase() == "key"; });

      if (key_it != attributes.end())
      {
        auto key_lc = key_it->second.lowercase();
        auto [category, id] = splitSoundKey(key_lc.raw());

        if (current_theme_ != nullptr)
        {
          if (id == "weak")
            current_params_ = &current_theme_->content.weak_params;
          else if (id == "mid")
            current_params_ = &current_theme_->content.mid_params;
          else if (id == "strong")
            current_params_ = &current_theme_->content.strong_params;
          else
            current_params_ = nullptr;
        }
      }
    }
  }
  catch(const std::exception& error)
  {
    std::string msg = makeErrorMessage(context, error.what());
    throw Glib::MarkupError {Glib::MarkupError::Code::INVALID_CONTENT, msg};
  }
}

void SoundThemeParser::on_end_element (Glib::Markup::ParseContext& context,
                                       const Glib::ustring& element_name)
{
  auto element_name_lc = element_name.lowercase();
  if (element_name_lc == "header"
      || element_name_lc == "content"
      || element_name_lc == "sounds"
      || element_name_lc == "tone"
      || element_name_lc == "noise")
  {
    current_block_.pop();
  }
  else if (element_name_lc == "sound-theme")
  {
    current_theme_ = nullptr;
    current_block_.pop();
  }
  else if (element_name_lc == "sound")
  {
    if (current_params_ != nullptr)
    {
      audio::clampSoundParameters(*current_params_);
      current_params_ = nullptr;
    }
    current_block_.pop();
  }
}

void SoundThemeParser::on_text(Glib::Markup::ParseContext& context,
                               const Glib::ustring& text)
{
  if (current_theme_ != nullptr && !current_block_.empty())
  {
    try {
      auto element_name_lc = context.get_element().lowercase();

      if (current_block_.top() == "header")
      {
        if (element_name_lc == "title")
          current_theme_->header.title = text;
        else if (element_name_lc == "description")
          current_theme_->header.description = text;
      }
      else if (current_block_.top() == "sound" && current_params_ != nullptr)
      {
        if (element_name_lc == "mix")
          current_params_->mix = stringToDouble(text);
        else if (element_name_lc == "pan")
          current_params_->pan = stringToDouble(text);
        else if (element_name_lc == "volume")
          current_params_->volume = stringToDouble(text);
      }
      else if (current_block_.top() == "tone" && current_params_ != nullptr)
      {
        if (element_name_lc == "pitch")
          current_params_->tone_pitch = stringToDouble(text);
        else if (element_name_lc == "timbre")
          current_params_->tone_timbre = stringToDouble(text);
        else if (element_name_lc == "detune")
          current_params_->tone_detune = stringToDouble(text);
        else if (element_name_lc == "attack")
          current_params_->tone_attack = stringToDouble(text);
        else if (element_name_lc == "attack-shape")
          current_params_->tone_attack_shape = stringToRampShape(text);
        else if (element_name_lc == "hold")
          current_params_->tone_hold = stringToDouble(text);
        else if (element_name_lc == "hold-shape")
          current_params_->tone_hold_shape = stringToHoldShape(text);
        else if (element_name_lc == "decay")
          current_params_->tone_decay = stringToDouble(text);
        else if (element_name_lc == "decay-shape")
          current_params_->tone_decay_shape = stringToRampShape(text);
      }
      else if (current_block_.top() == "noise" && current_params_ != nullptr)
      {
        if (element_name_lc == "cutoff")
          current_params_->noise_cutoff = stringToDouble(text);
        else if (element_name_lc == "attack")
          current_params_->noise_attack = stringToDouble(text);
        else if (element_name_lc == "attack-shape")
          current_params_->noise_attack_shape = stringToRampShape(text);
        else if (element_name_lc == "hold")
          current_params_->noise_hold = stringToDouble(text);
        else if (element_name_lc == "hold-shape")
          current_params_->noise_hold_shape = stringToHoldShape(text);
        else if (element_name_lc == "decay")
          current_params_->noise_decay = stringToDouble(text);
        else if (element_name_lc == "decay-shape")
          current_params_->noise_decay_shape = stringToRampShape(text);
      }
    }
    catch(const std::exception& error)
    {
      std::string msg = makeErrorMessage(context, error.what());
      throw Glib::MarkupError {Glib::MarkupError::Code::INVALID_CONTENT, msg};
    }
  }
}

namespace {

  void writeSoundThemeHeader(Glib::RefPtr<Gio::FileOutputStream> ostream,
                             const SoundTheme::Header& header)
  {
    ostream->write("    <header>\n");
    ostream->write("      <title>");
    auto text = Glib::Markup::escape_text(header.title);
    ostream->write(text);
    ostream->write("</title>\n");
    ostream->write("      <description>");
    text = Glib::Markup::escape_text(header.description);
    ostream->write(text);
    ostream->write("</description>\n");
    ostream->write("    </header>\n");
  }

  void writeSoundThemeParameters(Glib::RefPtr<Gio::FileOutputStream> ostream,
                                 const audio::SoundParameters& params)
  {
    ostream->write("        <tone>\n");
    ostream->write("          <pitch>");
    ostream->write(doubleToString(params.tone_pitch));
    ostream->write("</pitch>\n");
    ostream->write("          <timbre>");
    ostream->write(doubleToString(params.tone_timbre));
    ostream->write("</timbre>\n");
    ostream->write("          <detune>");
    ostream->write(doubleToString(params.tone_detune));
    ostream->write("</detune>\n");
    ostream->write("          <attack>");
    ostream->write(doubleToString(params.tone_attack));
    ostream->write("</attack>\n");
    ostream->write("          <attack-shape>");
    ostream->write(rampShapeToString(params.tone_attack_shape));
    ostream->write("</attack-shape>\n");
    ostream->write("          <hold>");
    ostream->write(doubleToString(params.tone_hold));
    ostream->write("</hold>\n");
    ostream->write("          <hold-shape>");
    ostream->write(holdShapeToString(params.tone_hold_shape));
    ostream->write("</hold-shape>\n");
    ostream->write("          <decay>");
    ostream->write(doubleToString(params.tone_decay));
    ostream->write("</decay>\n");
    ostream->write("          <decay-shape>");
    ostream->write(rampShapeToString(params.tone_decay_shape));
    ostream->write("</decay-shape>\n");
    ostream->write("        </tone>\n");
    ostream->write("        <noise>\n");
    ostream->write("          <cutoff>");
    ostream->write(doubleToString(params.noise_cutoff));
    ostream->write("</cutoff>\n");
    ostream->write("          <attack>");
    ostream->write(doubleToString(params.noise_attack));
    ostream->write("</attack>\n");
    ostream->write("          <attack-shape>");
    ostream->write(rampShapeToString(params.noise_attack_shape));
    ostream->write("</attack-shape>\n");
    ostream->write("          <hold>");
    ostream->write(doubleToString(params.noise_hold));
    ostream->write("</hold>\n");
    ostream->write("          <hold-shape>");
    ostream->write(holdShapeToString(params.noise_hold_shape));
    ostream->write("</hold-shape>\n");
    ostream->write("          <decay>");
    ostream->write(doubleToString(params.noise_decay));
    ostream->write("</decay>\n");
    ostream->write("          <decay-shape>");
    ostream->write(rampShapeToString(params.noise_decay_shape));
    ostream->write("</decay-shape>\n");
    ostream->write("        </noise>\n");
    ostream->write("        <mix>");
    ostream->write(doubleToString(params.mix));
    ostream->write("</mix>\n");
    ostream->write("        <pan>");
    ostream->write(doubleToString(params.pan));
    ostream->write("</pan>\n");
    ostream->write("        <volume>");
    ostream->write(doubleToString(params.volume));
    ostream->write("</volume>\n");
  }

  void writeSoundThemeContent(Glib::RefPtr<Gio::FileOutputStream> ostream,
                              const SoundTheme::Content& content)
  {
    ostream->write("    <content>\n");
    ostream->write("      <sounds>\n");
    ostream->write("        <sound key=\"accent/weak\">\n");
    writeSoundThemeParameters(ostream, content.weak_params);
    ostream->write("        </sound>\n");
    ostream->write("        <sound key=\"accent/mid\">\n");
    writeSoundThemeParameters(ostream, content.mid_params);
    ostream->write("        </sound>\n");
    ostream->write("        <sound key=\"accent/strong\">\n");
    writeSoundThemeParameters(ostream, content.strong_params);
    ostream->write("        </sound>\n");
    ostream->write("      </sounds>\n");
    ostream->write("    </content>\n");
  }
}//unnamed namespace

void SoundThemeWriter::writeEntry(Glib::RefPtr<Gio::FileOutputStream> ostream,
                                  const SoundTheme& theme, const Identifier& id)
{
  ostream->write("  <sound-theme id=\"");
  ostream->write(Glib::Markup::escape_text(id));
  ostream->write("\">\n");

  writeSoundThemeHeader(ostream, theme.header);
  writeSoundThemeContent(ostream, theme.content);

  ostream->write("  </sound-theme>\n");
}
