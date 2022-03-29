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
#include "Meter.h"
#include <map>
#include <algorithm>

namespace audio {

  struct SoundParameters
  {
    float timbre  {-1.0}; // [-1.0, 1.0]
    float pitch   {800};  // [20.0, 20000.0] (hertz)
    float volume  {1.0};  // [0.0, 100.0] (percent)
    float balance {0.0};  // [-1.0f, 1.0f]
  };

  inline bool operator==(const SoundParameters& lhs, const SoundParameters& rhs)
  {
    return lhs.timbre == rhs.timbre
      && lhs.pitch == rhs.pitch
      && lhs.volume == rhs.volume
      && lhs.balance == rhs.balance;
  }

  inline bool operator!=(const SoundParameters& lhs, const SoundParameters& rhs)
  { return !(lhs==rhs); }

  /**
   * Without mixing capabilities the time gap between two consecutive clicks
   * at maximum tempo (250 bpm) with the maximum number of beat division (4)
   * limits the click sound duration to 60 ms.
   */
  constexpr milliseconds kSoundDuration = 60ms;

  class SoundGenerator {
  public:
    SoundGenerator(const StreamSpec& spec = kDefaultSpec);

    /** Pre-allocate resources */
    void prepare(const StreamSpec& spec);

    /**
     * Allocates a new sound buffer and generates a sound with the given
     * sound parameters.
     */
    ByteBuffer generate(const SoundParameters& params);

    /**
     * Generates a sound with the given sound parameters. The buffer should be
     * able to hold 60ms (kSoundDuration) of audio data.
     * If the stream specification of the buffer does not fit the specification
     * of the SoundGenerator, the buffer will be reinterpreted but no further
     * heap allocations will take place to comply real-time demands.
     */
    void generate(ByteBuffer& buffer, const SoundParameters& params);

    /** See generate(const SoundParameter&) */
    ByteBuffer operator()(const SoundParameters& params)
      { return generate(params); }

    /** See generate(ByteBuffer&, const SoundParameter&) */
    void operator()(ByteBuffer& buffer, const SoundParameters& params)
      { generate(buffer, params); }

  private:
    StreamSpec spec_;
    ByteBuffer osc_buffer_;
    ByteBuffer noise_buffer_;
  };

  /**
   * @class SoundLibrary
   * @brief Generates and stores click sounds
   */
  template<typename KeyType = Accent>
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
#endif//GMetronome_Synthesizer_h
