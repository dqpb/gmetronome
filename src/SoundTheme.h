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

#include "Audio.h"
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
      if (settings)
      {
        target.tone_pitch = settings->get_double(settings::kKeySoundThemeTonePitch);
        target.tone_timbre = settings->get_double(settings::kKeySoundThemeToneTimbre);
        target.tone_detune = settings->get_double(settings::kKeySoundThemeToneDetune);
        target.tone_punch = settings->get_double(settings::kKeySoundThemeTonePunch);
        target.tone_decay = settings->get_double(settings::kKeySoundThemeToneDecay);
        target.percussion_cutoff = settings->get_double(settings::kKeySoundThemePercussionCutoff);
        target.percussion_clap = settings->get_boolean(settings::kKeySoundThemePercussionClap);
        target.percussion_punch = settings->get_double(settings::kKeySoundThemePercussionPunch);
        target.percussion_decay = settings->get_double(settings::kKeySoundThemePercussionDecay);
        target.mix = settings->get_double(settings::kKeySoundThemeMix);
        target.balance = settings->get_double(settings::kKeySoundThemeBalance);
        target.volume = settings->get_double(settings::kKeySoundThemeVolume);
        //...
      }
    }

  static void storeParameters(Glib::RefPtr<Gio::Settings> settings,
                              const audio::SoundParameters& source)
    {
      if (settings)
      {
        settings->set_double(settings::kKeySoundThemeTonePitch, source.tone_pitch);
        settings->set_double(settings::kKeySoundThemeToneTimbre, source.tone_timbre);
        settings->set_double(settings::kKeySoundThemeToneDetune, source.tone_detune);
        settings->set_double(settings::kKeySoundThemeTonePunch, source.tone_punch);
        settings->set_double(settings::kKeySoundThemeToneDecay, source.tone_decay);
        settings->set_double(settings::kKeySoundThemePercussionCutoff, source.percussion_cutoff);
        settings->set_boolean(settings::kKeySoundThemePercussionClap, source.percussion_clap);
        settings->set_double(settings::kKeySoundThemePercussionPunch, source.percussion_punch);
        settings->set_double(settings::kKeySoundThemePercussionDecay, source.percussion_decay);
        settings->set_double(settings::kKeySoundThemeMix, source.mix);
        settings->set_double(settings::kKeySoundThemeBalance, source.balance);
        settings->set_double(settings::kKeySoundThemeVolume, source.volume);
        //...
      }
    }

  static SoundTheme load(const SettingsTreeNode& settings_tree)
    {
      SoundTheme theme;
      theme.title = settings_tree.settings->get_string(settings::kKeySoundThemeTitle);

      auto& children = settings_tree.children;

      auto strong_params_settings
        = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
      auto mid_params_settings
        = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
      auto weak_params_settings
        = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

      loadParameters(strong_params_settings, theme.strong_params);
      loadParameters(mid_params_settings, theme.mid_params);
      loadParameters(weak_params_settings, theme.weak_params);

      return theme;
    }

  static void store(const SettingsTreeNode& settings_tree, const SoundTheme& theme)
    {
      settings_tree.settings->set_string(settings::kKeySoundThemeTitle, theme.title);

      auto& children = settings_tree.children;

      auto strong_params_settings
        = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
      auto mid_params_settings
        = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
      auto weak_params_settings
        = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

      storeParameters(strong_params_settings, theme.strong_params);
      storeParameters(mid_params_settings, theme.mid_params);
      storeParameters(weak_params_settings, theme.weak_params);
    }

  static bool paramsModified(Glib::RefPtr<Gio::Settings> settings)
    {
      bool m = false;
      Glib::Variant<double> dbl_value;
      Glib::Variant<bool> bool_value;

      if (settings)
      {
        m = m || settings->get_user_value(settings::kKeySoundThemeTonePitch, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneTimbre, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneDetune, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeTonePunch, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneDecay, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemePercussionCutoff, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemePercussionClap, bool_value);
        m = m || settings->get_user_value(settings::kKeySoundThemePercussionPunch, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemePercussionDecay, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeMix, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeBalance, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeVolume, dbl_value);
        //...
      }
      return m;
    }

  static bool modified(const SettingsTreeNode& settings_tree)
    {
      bool m = false;

      Glib::Variant<Glib::ustring> title_value;
      m = m || settings_tree.settings->get_user_value(settings::kKeySoundThemeTitle, title_value);

      auto& children = settings_tree.children;

      auto strong_params_settings
        = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
      auto mid_params_settings
        = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
      auto weak_params_settings
        = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

      if (!m) m = paramsModified(strong_params_settings);
      if (!m) m = paramsModified(mid_params_settings);
      if (!m) m = paramsModified(weak_params_settings);

      return m;
    }
};

#endif//GMetronome_SoundTheme_h
