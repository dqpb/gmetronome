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
#include <string>

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
  static void loadParameters(Glib::RefPtr<Gio::Settings> settings,
                             audio::SoundParameters& target)
    {
      target.pitch = settings->get_double(settings::kKeySoundThemeTonalPitch);
      target.timbre = settings->get_double(settings::kKeySoundThemeTonalTimbre);
      target.detune = settings->get_double(settings::kKeySoundThemeTonalDetune);
      target.punch = settings->get_double(settings::kKeySoundThemeTonalPunch);
      target.decay = settings->get_double(settings::kKeySoundThemeTonalDecay);
      //...
    }

  static void storeParameters(Glib::RefPtr<Gio::Settings> settings,
                              const audio::SoundParameters& source)
    {
      settings->set_double(settings::kKeySoundThemeTonalPitch, source.pitch);
      settings->set_double(settings::kKeySoundThemeTonalTimbre, source.timbre);
      settings->set_double(settings::kKeySoundThemeTonalDetune, source.detune);
      settings->set_double(settings::kKeySoundThemeTonalPunch, source.punch);
      settings->set_double(settings::kKeySoundThemeTonalDecay, source.decay);
      //...
    }

  static SoundTheme load(Glib::RefPtr<Gio::Settings> settings)
    {
      SoundTheme theme;
      theme.title = settings->get_string(settings::kKeySoundThemeTitle);

      Glib::RefPtr<Gio::Settings> params_settings;

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeStrongParamsBasename);
      loadParameters(params_settings, theme.strong_params);

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeMidParamsBasename);
      loadParameters(params_settings, theme.mid_params);

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeWeakParamsBasename);
      loadParameters(params_settings, theme.weak_params);

      return theme;
    }

  static void store(const SoundTheme& theme, Glib::RefPtr<Gio::Settings> settings)
    {
      settings->set_string(settings::kKeySoundThemeTitle, theme.title);

      Glib::RefPtr<Gio::Settings> params_settings;

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeStrongParamsBasename);
      storeParameters(params_settings, theme.strong_params);

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeMidParamsBasename);
      storeParameters(params_settings, theme.mid_params);

      params_settings = settings->get_child(settings::kSchemaPathSoundThemeWeakParamsBasename);
      storeParameters(params_settings, theme.weak_params);
    }

  static bool paramsModified(Glib::RefPtr<Gio::Settings> params_settings)
    {
      bool m = false;
      Glib::Variant<double> dbl_value;

      m = m || params_settings->get_user_value(settings::kKeySoundThemeTonalPitch, dbl_value);
      m = m || params_settings->get_user_value(settings::kKeySoundThemeTonalTimbre, dbl_value);
      m = m || params_settings->get_user_value(settings::kKeySoundThemeTonalDetune, dbl_value);
      m = m || params_settings->get_user_value(settings::kKeySoundThemeTonalPunch, dbl_value);
      m = m || params_settings->get_user_value(settings::kKeySoundThemeTonalDecay, dbl_value);
      //...

      return m;
    }

  static bool modified(Glib::RefPtr<Gio::Settings> settings)
    {
      bool m = false;

      Glib::Variant<Glib::ustring> title_value;
      m = m || settings->get_user_value(settings::kKeySoundThemeTitle, title_value);

      Glib::RefPtr<Gio::Settings> params_settings;

      if (!m) {
        params_settings = settings->get_child(settings::kSchemaPathSoundThemeStrongParamsBasename);
        m = paramsModified(params_settings);
      }
      if (!m) {
        params_settings = settings->get_child(settings::kSchemaPathSoundThemeMidParamsBasename);
        m = paramsModified(params_settings);
      }
      if (!m) {
        params_settings = settings->get_child(settings::kSchemaPathSoundThemeWeakParamsBasename);
        m = paramsModified(params_settings);
      }
      return m;
    }
};

#endif//GMetronome_SoundTheme_h
