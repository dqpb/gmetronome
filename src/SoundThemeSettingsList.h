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

#ifndef GMetronome_SoundThemeSettingsList_h
#define GMetronome_SoundThemeSettingsList_h

#include "SoundTheme.h"
#include "SettingsList.h"
#include "Synthesizer.h"

#include <string>
#include <glibmm/variant.h>
#include <glibmm/i18n.h>

template<>
struct SettingsListDelegate<SoundTheme>
{
  static void loadParameters(Glib::RefPtr<Gio::Settings> settings,
                             audio::SoundParameters& target)
    {
      if (settings)
      {
        target.tone_pitch  = settings->get_double(settings::kKeySoundThemeTonePitch);
        target.tone_timbre = settings->get_double(settings::kKeySoundThemeToneTimbre);
        target.tone_detune = settings->get_double(settings::kKeySoundThemeToneDetune);

        target.tone_attack = settings->get_double(settings::kKeySoundThemeToneAttack);
        target.tone_attack_shape = static_cast<audio::EnvelopeRampShape>(
          settings->get_enum(settings::kKeySoundThemeToneAttackShape));

        target.tone_hold = settings->get_double(settings::kKeySoundThemeToneHold);
        target.tone_hold_shape = static_cast<audio::EnvelopeHoldShape>(
          settings->get_enum(settings::kKeySoundThemeToneHoldShape));

        target.tone_decay = settings->get_double(settings::kKeySoundThemeToneDecay);
        target.tone_decay_shape = static_cast<audio::EnvelopeRampShape>(
          settings->get_enum(settings::kKeySoundThemeToneDecayShape));

        target.noise_cutoff = settings->get_double(settings::kKeySoundThemeNoiseCutoff);

        target.noise_attack = settings->get_double(settings::kKeySoundThemeNoiseAttack);
        target.noise_attack_shape = static_cast<audio::EnvelopeRampShape>(
          settings->get_enum(settings::kKeySoundThemeNoiseAttackShape));

        target.noise_hold = settings->get_double(settings::kKeySoundThemeNoiseHold);
        target.noise_hold_shape = static_cast<audio::EnvelopeHoldShape>(
          settings->get_enum(settings::kKeySoundThemeNoiseHoldShape));

        target.noise_decay = settings->get_double(settings::kKeySoundThemeNoiseDecay);
        target.noise_decay_shape = static_cast<audio::EnvelopeRampShape>(
          settings->get_enum(settings::kKeySoundThemeNoiseDecayShape));

        target.mix = settings->get_double(settings::kKeySoundThemeMix);
        target.pan = settings->get_double(settings::kKeySoundThemePan);
        target.volume = settings->get_double(settings::kKeySoundThemeVolume);
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

        settings->set_double(settings::kKeySoundThemeToneAttack, source.tone_attack);
        settings->set_enum(settings::kKeySoundThemeToneAttackShape,
                           static_cast<int>(source.tone_attack_shape));

        settings->set_double(settings::kKeySoundThemeToneHold, source.tone_hold);
        settings->set_enum(settings::kKeySoundThemeToneHoldShape,
                           static_cast<int>(source.tone_hold_shape));

        settings->set_double(settings::kKeySoundThemeToneDecay, source.tone_decay);
        settings->set_enum(settings::kKeySoundThemeToneDecayShape,
                           static_cast<int>(source.tone_decay_shape));

        settings->set_double(settings::kKeySoundThemeNoiseCutoff, source.noise_cutoff);

        settings->set_double(settings::kKeySoundThemeNoiseAttack, source.noise_attack);
        settings->set_enum(settings::kKeySoundThemeNoiseAttackShape,
                           static_cast<int>(source.noise_attack_shape));

        settings->set_double(settings::kKeySoundThemeNoiseHold, source.noise_hold);
        settings->set_enum(settings::kKeySoundThemeNoiseHoldShape,
                           static_cast<int>(source.noise_hold_shape));

        settings->set_double(settings::kKeySoundThemeNoiseDecay, source.noise_decay);
        settings->set_enum(settings::kKeySoundThemeNoiseDecayShape,
                           static_cast<int>(source.noise_decay_shape));

        settings->set_double(settings::kKeySoundThemeMix, source.mix);
        settings->set_double(settings::kKeySoundThemePan, source.pan);
        settings->set_double(settings::kKeySoundThemeVolume, source.volume);
      }
    }

  static SoundTheme load(const SettingsTreeNode& settings_tree)
    {
      SoundTheme theme;
      theme.header.title = settings_tree.settings->get_string(settings::kKeySoundThemeTitle);

      auto& children = settings_tree.children;

      auto strong_params_settings
        = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
      auto mid_params_settings
        = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
      auto weak_params_settings
        = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

      loadParameters(strong_params_settings, theme.content.strong_params);
      loadParameters(mid_params_settings, theme.content.mid_params);
      loadParameters(weak_params_settings, theme.content.weak_params);

      return theme;
    }

  static void store(const SettingsTreeNode& settings_tree, const SoundTheme& theme)
    {
      settings_tree.settings->set_string(settings::kKeySoundThemeTitle, theme.header.title);

      auto& children = settings_tree.children;

      auto strong_params_settings
        = children.at(settings::kSchemaPathSoundThemeStrongParamsBasename).settings;
      auto mid_params_settings
        = children.at(settings::kSchemaPathSoundThemeMidParamsBasename).settings;
      auto weak_params_settings
        = children.at(settings::kSchemaPathSoundThemeWeakParamsBasename).settings;

      storeParameters(strong_params_settings, theme.content.strong_params);
      storeParameters(mid_params_settings, theme.content.mid_params);
      storeParameters(weak_params_settings, theme.content.weak_params);
    }

  static bool paramsModified(Glib::RefPtr<Gio::Settings> settings)
    {
      bool m = false;
      Glib::Variant<double> dbl_value;
      Glib::Variant<int> int_value;

      if (settings)
      {
        m = m || settings->get_user_value(settings::kKeySoundThemeTonePitch, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneTimbre, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneDetune, dbl_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeToneAttack, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneAttackShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeToneHold, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneHoldShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeToneDecay, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeToneDecayShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseCutoff, dbl_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseAttack, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseAttackShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseHold, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseHoldShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseDecay, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeNoiseDecayShape, int_value);

        m = m || settings->get_user_value(settings::kKeySoundThemeMix, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemePan, dbl_value);
        m = m || settings->get_user_value(settings::kKeySoundThemeVolume, dbl_value);
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

#endif//GMetronome_SoundThemeSettingsList_h
