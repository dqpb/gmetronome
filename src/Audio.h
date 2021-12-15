/*
 * Copyright (C) 2020 The GMetronome Team
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

#ifndef GMetronome_Audio_h
#define GMetronome_Audio_h

#include <vector>
#include <chrono>
#include <memory>

#include "Settings.h"

namespace audio {

  using std::chrono::seconds;  
  using std::chrono::microseconds;  
  using std::chrono::milliseconds;
  
  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""us;

  /**
   */
  enum class SampleFormat
  {
    // U8,
    // ALAW,
    // ULAW,
    S16LE,
    // S16BE,
    // Float32LE,
    // Float32BE,
    // S32LE,
    // S32BE,
    // S24LE,
    // S24BE,
    // S24_32LE,
    // S24_32BE
    // Max,
    // Invalid = -1
  };

  using SampleRate = uint32_t;

  enum class Endianness
  {
    Little,
    Big,
    Invalid
  };
  
  struct StreamSpec
  {
    SampleFormat  format;
    SampleRate    rate;
    unsigned int  channels;
  };

  constexpr SampleFormat kDefaultFormat   = SampleFormat::S16LE;
  constexpr SampleRate   kDefaultRate     = 44100;
  constexpr unsigned int kDefaultChannels = 2;

  constexpr StreamSpec kDefaultSpec =
  {
    kDefaultFormat,
    kDefaultRate,
    kDefaultChannels
  };

  /** Returns the size of a sample with the specific sample type. */
  size_t sampleSize(SampleFormat format);
  
  /** Return the endianness of a sample format. */
  Endianness endianness(SampleFormat format);
  
  /** Return the size of a frame with a given sample specification. */
  size_t frameSize(StreamSpec spec);
  
  /** 
   * Calculates the number of frames that are required for the specified 
   * time. The return value will always be rounded down for non-integral 
   * return values. 
   */
  size_t usecsToFrames(microseconds usecs, const StreamSpec& spec);
  
  /**
   * Calculate the time the number of frames take to play with the 
   * specified sample type. The return value will always be rounded 
   * down for non-integral return values.
   */
  microseconds framesToUsecs(size_t frames, const StreamSpec& spec);
  
  /** 
   * Calculates the number of bytes that are required for the specified 
   * time. The return value will always be rounded down for non-integral 
   * return values. 
   */
  size_t usecsToBytes(microseconds usecs, const StreamSpec& spec);
  
  /**
   * Calculate the time the number of bytes take to play with the 
   * specified sample type. The return value will always be rounded 
   * down for non-integral return values.
   */
  microseconds bytesToUsecs(size_t bytes, const StreamSpec& spec);

}//namespace audio
#endif//GMetronome_Audio_h
