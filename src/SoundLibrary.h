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

#ifndef GMetronome_SoundLibrary_h
#define GMetronome_SoundLibrary_h

#include "AudioBuffer.h"
#include "Synthesizer.h"
#include <algorithm>
#include <map>

namespace audio {

    /**
   * @class SoundLibrary
   * @brief Generates and stores click sounds
   */
  template<typename KeyType>
  class SoundLibrary {
  public:
    SoundLibrary(const StreamSpec& spec = kDefaultSpec);

    /**
     * Invalidates the sound cache and reallocates resources for the
     * new stream specification. The sounds will be regenerated when
     * update() or get() is called.
     */
    void reconfigure(StreamSpec spec);

    /** Removes all entries in the sound library. */
    void clear();

    /**
     * Adjust the sound parameters and invalidates the sound cache for
     * a given key. The sound will be regenerated when update() or get()
     * is called.
     */
    void adjust(KeyType key, const SoundParameters& params);

    /** Returns the sound for a given key. */
    const ByteBuffer& get(KeyType key);
    const ByteBuffer& operator[](const KeyType& key);

    /** Regenerates the sound and updates the sound cache for a given key. */
    void update(KeyType key);

    /** Update all registered sounds. */
    void update();

  private:
    StreamSpec spec_;
    SoundGenerator generator_;

    struct SoundMapEntry_
    {
      SoundParameters params;
      ByteBuffer sound;
      bool need_update {true};
    };
    std::map<KeyType, SoundMapEntry_> sound_map_;

    void update(SoundMapEntry_& entry);
  };


  template<typename KeyType>
  SoundLibrary<KeyType>::SoundLibrary(const StreamSpec& spec)
    : spec_{spec},
      generator_{spec}
  {}

  template<typename KeyType>
  void SoundLibrary<KeyType>::reconfigure(StreamSpec spec)
  {
    if (spec != spec_)
    {
      generator_.prepare(spec);
      std::transform(sound_map_.begin(), sound_map_.end(), sound_map_.begin(),
                     [] (auto& p) { p.second.need_update = true; });
      spec_ = spec;
    }
  }

  template<typename KeyType>
  void SoundLibrary<KeyType>::clear()
  {
    sound_map_.clear();
  }

  template<typename KeyType>
  void SoundLibrary<KeyType>::adjust(KeyType key, const SoundParameters& params)
  {
    auto& entry = sound_map_[key];
    if (entry.params != params)
    {
      entry.params = params;
      entry.need_update = true;
    }
  }

  template<typename KeyType>
  const ByteBuffer& SoundLibrary<KeyType>::get(KeyType key)
  {
    auto& entry = sound_map_[key];

    if (entry.need_update)
      update(key);

    return entry.sound;
  }

  template<typename KeyType>
  const ByteBuffer& SoundLibrary<KeyType>::operator[](const KeyType& key)
  {
    return get(key);
  }

  template<typename KeyType>
  void SoundLibrary<KeyType>::update(KeyType key)
  {
    auto& entry = sound_map_[key];
    if (entry.need_update)
      update(entry);
  }

  template<typename KeyType>
  void SoundLibrary<KeyType>::update()
  {
    // TODO: check, if this can be parallelized
    for (auto& p : sound_map_)
      if (p.second.need_update)
        update(p.second);
  }

  template<typename KeyType>
  void SoundLibrary<KeyType>::update(SoundMapEntry_& entry)
  {
    if (!entry.need_update)
      return;

    if (entry.sound.spec() != spec_
        || entry.sound.frames() < usecsToFrames(kSoundDuration, spec_))
    {
      entry.sound.resize(spec_, kSoundDuration);
    }
    generator_.generate(entry.sound, entry.params);

    entry.need_update = false;
  }

}//namespace audio
#endif//GMetronome_SoundLibrary_h
