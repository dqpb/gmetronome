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
#include "Filter.h"
#include "AudioBuffer.h"
#include "WavetableLibrary.h"

#include <tuple>

#ifndef NDEBUG
# include <iostream>
#endif

namespace audio {

  struct SoundParameters
  {
    float tonal_pitch        {900};    // [40.0f, 10000.0f] (hertz)
    float tonal_timbre       {1.0};    // [0.0f, 3.0f]
    float tonal_detune       {0.0};    // [0.0f, 100.0f] (cents)
    float tonal_punch        {0.5};    // [0.0f, 1.0f]
    float tonal_decay        {0.5};    // [0.0f, 1.0f]
    float percussive_tone    {0.5};    // [0.0f, 1.0f]
    float percussive_punch   {0.5};    // [0.0f, 1.0f]
    float percussive_decay   {0.5};    // [0.0f, 1.0f]
    bool  percussive_clap    {false};
    float mix                {0.0};    // [-1.0f, 1.0f]
    bool  bell               {false};
    float bell_volume        {1.0};    // [ 0.0f, 1.0f]
    float balance            {0.0};    // [-1.0f, 1.0f]
    float volume             {1.0};    // [ 0.0f, 1.0f]
  };

  inline bool operator==(const SoundParameters& lhs, const SoundParameters& rhs)
  {
    return lhs.tonal_pitch == rhs.tonal_pitch
      && lhs.tonal_timbre == rhs.tonal_timbre
      && lhs.tonal_detune == rhs.tonal_detune
      && lhs.tonal_punch == rhs.tonal_punch
      && lhs.tonal_decay == rhs.tonal_decay
      && lhs.percussive_tone == rhs.percussive_tone
      && lhs.percussive_punch == rhs.percussive_punch
      && lhs.percussive_decay == rhs.percussive_decay
      && lhs.percussive_clap == rhs.percussive_clap
      && lhs.mix == rhs.mix
      && lhs.bell == rhs.bell
      && lhs.bell_volume == rhs.bell_volume
      && lhs.balance == rhs.balance
      && lhs.volume == rhs.volume;
  }

  inline bool operator!=(const SoundParameters& lhs, const SoundParameters& rhs)
  { return !(lhs==rhs); }

  /**
   * Without mixing capabilities the time gap between two consecutive clicks
   * at maximum tempo (250 bpm) with the maximum number of beat division (4)
   * limits the click sound duration to 60 ms.
   */
  constexpr milliseconds kSoundDuration = 60ms;

  /** Implements the requirements of a builder for ObjectLibrary. */
  class Synthesizer {
  public:
    Synthesizer(const StreamSpec& spec = kDefaultSpec);

    /** Pre-allocate resources */
    void prepare(const StreamSpec& spec);

    /**
     * Allocates a new sound buffer and generates a sound with the given
     * sound parameters.
     */
    ByteBuffer create(const SoundParameters& params);

    /**
     * Generates a sound with the given sound parameters. The buffer should be
     * able to hold 60ms (kSoundDuration) of audio data.
     * If the stream specification of the buffer does not fit the specification
     * of the Synthesizer, the buffer will be resized.
     */
    void update(ByteBuffer& buffer, const SoundParameters& params);

  public:
    struct SineRecipe : public WavetableRecipe
    {
      SineRecipe()
        { /* nothing */ }
      ~SineRecipe() override
        { /* nothing */ }
      size_t preferredPages(SampleRate rate) const override
        { return 1; };
      size_t preferredBasePageSize(SampleRate rate) const override
        { return 2048; }
      Wavetable::PageResize preferredPageResize(SampleRate rate) const override
        { return Wavetable::PageResize::kNoResize; }
      float preferredBase(SampleRate rate) const override
        { return 40.0f; }
      Wavetable::PageRange preferredRange(SampleRate rate) const override
        { return Wavetable::PageRange::kFull; }

      void fillPage(SampleRate rate,
                    size_t page,
                    float base,
                    Wavetable::Page::iterator begin,
                    Wavetable::Page::iterator end) const override;
    };

    struct TriangleRecipe : public WavetableRecipe
    {
      void fillPage(SampleRate rate,
                    size_t page,
                    float base,
                    Wavetable::Page::iterator begin,
                    Wavetable::Page::iterator end) const override;
    };

    struct SawtoothRecipe : public WavetableRecipe
    {
      void fillPage(SampleRate rate,
                    size_t page,
                    float base,
                    Wavetable::Page::iterator begin,
                    Wavetable::Page::iterator end) const override;
    };

    struct SquareRecipe : public WavetableRecipe
    {
      void fillPage(SampleRate rate,
                    size_t page,
                    float base,
                    Wavetable::Page::iterator begin,
                    Wavetable::Page::iterator end) const override;
    };

  private:
    StreamSpec spec_;
    ByteBuffer osc_buffer_;
    ByteBuffer noise_buffer_;

    static constexpr int kSineTable     = 0;
    static constexpr int kTriangleTable = 1;
    static constexpr int kSawtoothTable = 2;
    static constexpr int kSquareTable   = 3;
    static constexpr int kNoiseTable    = 4;

    WavetableLibrary wavetables_;

    std::tuple<filter::Automation, microseconds, microseconds>
    buildEnvelope(float attack, float decay, bool clap) const;
  };

}//namespace audio
#endif//GMetronome_Synthesizer_h
