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

namespace audio {

  struct SoundParameters
  {
    float tone_pitch        {1000};   // [40.0f, 10000.0f] (hertz)
    float tone_timbre       {0.0};    // [0.0f, 3.0f]
    float tone_detune       {0.0};    // [0.0f, 100.0f] (cents)
    float tone_punch        {0.5};    // [0.0f, 1.0f]
    float tone_decay        {0.5};    // [0.0f, 1.0f]
    float percussion_cutoff {0.5};    // [40.0f, 10000.0f] (hertz)
    bool  percussion_clap   {false};
    float percussion_punch  {0.5};    // [0.0f, 1.0f]
    float percussion_decay  {0.5};    // [0.0f, 1.0f]
    float mix               {0.0};    // [-100.0f, 100.0f] (percent)
    float balance           {0.0};    // [-100.0f, 100.0f] (percent)
    float volume            {75.0};   // [0.0f, 100.0f] (percent)
  };

  inline bool operator==(const SoundParameters& lhs, const SoundParameters& rhs)
  {
    return lhs.tone_pitch == rhs.tone_pitch
      && lhs.tone_timbre == rhs.tone_timbre
      && lhs.tone_detune == rhs.tone_detune
      && lhs.tone_punch == rhs.tone_punch
      && lhs.tone_decay == rhs.tone_decay
      && lhs.percussion_cutoff == rhs.percussion_cutoff
      && lhs.percussion_clap == rhs.percussion_clap
      && lhs.percussion_punch == rhs.percussion_punch
      && lhs.percussion_decay == rhs.percussion_decay
      && lhs.mix == rhs.mix
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

    using NoiseFilterPipe = decltype(
        filter::std::Zero()
      | filter::std::Noise()
      | filter::std::Lowpass()
      | filter::std::Gain()
    );

    NoiseFilterPipe noise_pipe_;

    std::tuple<filter::Automation, microseconds, microseconds>
    buildEnvelope(float attack, float decay, bool clap) const;
  };

}//namespace audio
#endif//GMetronome_Synthesizer_h
