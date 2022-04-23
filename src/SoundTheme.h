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

#ifndef GMetronome_SoundTheme_h
#define GMetronome_SoundTheme_h

#include "Settings.h"
#include "SettingsList.h"
#include "Synthesizer.h"
#include "Meter.h"

#include <glibmm/variant.h>
#include <glibmm/i18n.h>
#include <tuple>
#include <string>
#include <array>

struct SoundTheme
{
  // default title for new sound themes
  static inline const std::string kDefaultTitle = N_("New Sound Theme");

  // placeholder title for untitled sound themes
  static inline const std::string kDefaultTitlePlaceholder = N_("Untitled");

  // title of duplicated sound themes, %1 will be replaced by the old title
  static inline const std::string kDefaultTitleDuplicate = _("%1 (copy)");

  std::string title = kDefaultTitle;

  audio::SoundParameters strong_params;
  audio::SoundParameters mid_params;
  audio::SoundParameters weak_params;
};

template<>
struct SettingsListDelegate<SoundTheme>
{
  using ParamsTuple = settings::SoundParametersTuple;

  static void copyParameters(const ParamsTuple& source,
                             audio::SoundParameters& target)
    {
      std::tie(target.pitch,
               target.timbre,
               target.detune,
               target.punch,
               target.decay,
               target.bell,
               target.bell_volume,
               target.balance,
               target.volume) = source;
    }

  static void copyParameters(const audio::SoundParameters& source,
                             ParamsTuple& target)
    {
      target = std::tie(source.pitch,
                        source.timbre,
                        source.detune,
                        source.punch,
                        source.decay,
                        source.bell,
                        source.bell_volume,
                        source.balance,
                        source.volume);
    }

  static SoundTheme load(Glib::RefPtr<Gio::Settings> settings)
    {
      SoundTheme theme;
      theme.title = settings->get_string(settings::kKeySoundThemeTitle);

      Glib::Variant<ParamsTuple> value;

      settings->get_value(settings::kKeySoundThemeStrongParams, value);
      copyParameters(value.get(), theme.strong_params);

      settings->get_value(settings::kKeySoundThemeMidParams, value);
      copyParameters(value.get(), theme.mid_params);

      settings->get_value(settings::kKeySoundThemeWeakParams, value);
      copyParameters(value.get(), theme.weak_params);

      return theme;
    }

  static void store(const SoundTheme& theme, Glib::RefPtr<Gio::Settings> settings)
    {
      settings->set_string(settings::kKeySoundThemeTitle, theme.title);

      ParamsTuple tpl;

      copyParameters(theme.strong_params, tpl);
      settings->set_value(settings::kKeySoundThemeStrongParams,
                          Glib::Variant<ParamsTuple>::create(tpl));

      copyParameters(theme.mid_params, tpl);
      settings->set_value(settings::kKeySoundThemeMidParams,
                          Glib::Variant<ParamsTuple>::create(tpl));

      copyParameters(theme.weak_params, tpl);
      settings->set_value(settings::kKeySoundThemeWeakParams,
                          Glib::Variant<ParamsTuple>::create(tpl));
    }

  static bool modified(Glib::RefPtr<Gio::Settings> settings)
    {
      bool m = false;

      Glib::Variant<Glib::ustring> title_value;
      m = m || settings->get_user_value(settings::kKeySoundThemeTitle, title_value);

      Glib::Variant<ParamsTuple> params_value;
      m = m || settings->get_user_value(settings::kKeySoundThemeStrongParams, params_value);
      m = m || settings->get_user_value(settings::kKeySoundThemeMidParams, params_value);
      m = m || settings->get_user_value(settings::kKeySoundThemeWeakParams, params_value);

      return m;
    }
};

#endif//GMetronome_SoundTheme_h
