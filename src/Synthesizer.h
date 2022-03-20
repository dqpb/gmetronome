/*
 * Copyright (C) 2021, 2022 The GMetronome Team
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

#ifndef GMetronome_Synthesizer_h
#define GMetronome_Synthesizer_h

#include "Audio.h"
#include "AudioBuffer.h"
#include <vector>

namespace audio {
namespace synth {

  ByteBuffer generateClick(const StreamSpec& spec,
                           float timbre = -1.0,
                           float pitch = 800,
                           float volume = 1.0,
                           float balance = 0.0);

  using SoundPackage = std::vector<ByteBuffer>;

  /**
   * @brief Generates a collection of slightly different sounds.
   *
   * This function calls generateClick() to create the sounds, but
   * tries to parallelize the sound generation in accordance with
   * the system capabilities.
   */
  SoundPackage generateClickPackage(const StreamSpec& spec,
                                    float timbre = -1.0,
                                    float pitch = 800,
                                    float volume = 1.0,
                                    float balance = 0.0,
                                    std::size_t package_size = 3);
}//namespace synth
}//namespace audio
#endif//GMetronome_Synthesizer_h
