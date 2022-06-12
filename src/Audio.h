/*
 * Copyright (C) 2020-2022 The GMetronome Team
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <chrono>
#include <algorithm>
#include <vector>
#include <cmath>

namespace audio {

  using std::chrono::seconds;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;

  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""us;

  /**
   */
  enum class SampleFormat : unsigned char
  {
    kU8 = 0,
    kS8,
    kS16LE,
    kS16BE,
    kU16LE,
    kU16BE,
    kS32LE,
    kS32BE,
    kFloat32LE,
    kFloat32BE,
    // kS24LE,
    // kS24BE,
    // kS24_32LE,
    // kS24_32BE,
    // kALAW,
    // kULAW,
    kUnknown = 0xf0
  };

  enum class Endian : unsigned char
  {
    kLittle  = 0,
    kBig     = 1,
    kUnknown = 0xf0
  };

  enum class Signedness : unsigned char
  {
    kSigned   = 0,
    kUnsigned = 1,
    kUnknown  = 0xf0
  };

  enum class SampleDataType : unsigned char
  {
    kIntegral      = 0,
    kFloatingPoint = 1,
    kUnknown       = 0xf0
  };

  using SampleRate = uint32_t;

  struct StreamSpec
  {
    SampleFormat  format;
    SampleRate    rate;
    unsigned int  channels;
  };

  bool operator==(const StreamSpec& lhs, const StreamSpec& rhs);
  bool operator!=(const StreamSpec& lhs, const StreamSpec& rhs);

  constexpr SampleFormat kDefaultFormat   = SampleFormat::kS16LE;
  constexpr SampleRate   kDefaultRate     = 44100;
  constexpr unsigned int kDefaultChannels = 2;

  constexpr StreamSpec kDefaultSpec =
  {
    kDefaultFormat,
    kDefaultRate,
    kDefaultChannels
  };

  // SampleValueType
  template<SampleFormat Format> struct SampleValueType
  {};
  template<> struct SampleValueType<SampleFormat::kU8>
  { using type = uint8_t; };
  template<> struct SampleValueType<SampleFormat::kS8>
  { using type = int8_t; };
  template<> struct SampleValueType<SampleFormat::kS16LE>
  { using type = int16_t; };
  template<> struct SampleValueType<SampleFormat::kS16BE>
  { using type = int16_t; };
  template<> struct SampleValueType<SampleFormat::kU16LE>
  { using type = uint16_t; };
  template<> struct SampleValueType<SampleFormat::kU16BE>
  { using type = uint16_t; };
  template<> struct SampleValueType<SampleFormat::kS32LE>
  { using type = int32_t; };
  template<> struct SampleValueType<SampleFormat::kS32BE>
  { using type = int32_t; };
  template<> struct SampleValueType<SampleFormat::kFloat32LE>
  { using type = float; };
  template<> struct SampleValueType<SampleFormat::kFloat32BE>
  { using type = float; };
  template<> struct SampleValueType<SampleFormat::kUnknown>
  { using type = void; };

  /** Returns the size of a sample with the specific sample type. */
  constexpr int sampleSize(const SampleFormat format)
  {
    switch (format)
    {
    case SampleFormat::kU8: return 1; break;
    case SampleFormat::kS8: return 1; break;
    case SampleFormat::kS16LE: return 2; break;
    case SampleFormat::kS16BE: return 2; break;
    case SampleFormat::kU16LE: return 2; break;
    case SampleFormat::kU16BE: return 2; break;
    case SampleFormat::kS32LE: return 4; break;
    case SampleFormat::kS32BE: return 4; break;
    case SampleFormat::kFloat32LE: return 4; break;
    case SampleFormat::kFloat32BE: return 4; break;
    case SampleFormat::kUnknown:
      [[fallthrough]];
    default: return 0;
      break;
    };
  }

  /** Returns the endianness of a sample format. */
  constexpr Endian sampleEndian(const SampleFormat format)
  {
    switch (format)
    {
    case SampleFormat::kU8: return Endian::kUnknown; break;
    case SampleFormat::kS8: return Endian::kUnknown; break;
    case SampleFormat::kS16LE: return Endian::kLittle; break;
    case SampleFormat::kS16BE: return Endian::kBig; break;
    case SampleFormat::kU16LE: return Endian::kLittle; break;
    case SampleFormat::kU16BE: return Endian::kBig; break;
    case SampleFormat::kS32LE: return Endian::kLittle; break;
    case SampleFormat::kS32BE: return Endian::kBig; break;
    case SampleFormat::kFloat32LE: return Endian::kLittle; break;
    case SampleFormat::kFloat32BE: return Endian::kBig; break;
    case SampleFormat::kUnknown:
      [[fallthrough]];
    default: return Endian::kUnknown;
      break;
    };
  }

  constexpr bool isLittleEndian(SampleFormat format)
  { return sampleEndian(format) == Endian::kLittle; }
  constexpr bool isBigEndian(SampleFormat format)
  { return sampleEndian(format) == Endian::kBig; }

  constexpr Signedness sampleSignedness(const SampleFormat format)
  {
    switch (format)
    {
    case SampleFormat::kU8: return Signedness::kUnsigned; break;
    case SampleFormat::kS8: return Signedness::kSigned; break;
    case SampleFormat::kS16LE: return Signedness::kSigned; break;
    case SampleFormat::kS16BE: return Signedness::kSigned; break;
    case SampleFormat::kU16LE: return Signedness::kUnsigned; break;
    case SampleFormat::kU16BE: return Signedness::kUnsigned; break;
    case SampleFormat::kS32LE: return Signedness::kSigned; break;
    case SampleFormat::kS32BE: return Signedness::kSigned; break;
    case SampleFormat::kFloat32LE: return Signedness::kSigned; break;
    case SampleFormat::kFloat32BE: return Signedness::kSigned; break;
    case SampleFormat::kUnknown:
      [[fallthrough]];
    default: return Signedness::kUnknown;
      break;
    };
  }

  constexpr bool isSigned(SampleFormat format)
  { return sampleSignedness(format) == Signedness::kSigned; }
  constexpr bool isUnsigned(SampleFormat format)
  { return sampleSignedness(format) == Signedness::kUnsigned; }

  constexpr SampleDataType sampleDataType(const SampleFormat format)
  {
    switch (format)
    {
    case SampleFormat::kU8: return SampleDataType::kIntegral; break;
    case SampleFormat::kS8: return SampleDataType::kIntegral; break;
    case SampleFormat::kS16LE: return SampleDataType::kIntegral; break;
    case SampleFormat::kS16BE: return SampleDataType::kIntegral; break;
    case SampleFormat::kU16LE: return SampleDataType::kIntegral; break;
    case SampleFormat::kU16BE: return SampleDataType::kIntegral; break;
    case SampleFormat::kS32LE: return SampleDataType::kIntegral; break;
    case SampleFormat::kS32BE: return SampleDataType::kIntegral; break;
    case SampleFormat::kFloat32LE: return SampleDataType::kFloatingPoint; break;
    case SampleFormat::kFloat32BE: return SampleDataType::kFloatingPoint; break;
    case SampleFormat::kUnknown:
      [[fallthrough]];
    default: return SampleDataType::kUnknown;
      break;
    };
  }

  constexpr bool isIntegral(SampleFormat format)
  { return sampleDataType(format) == SampleDataType::kIntegral; }
  constexpr bool isFloatingPoint(SampleFormat format)
  { return sampleDataType(format) == SampleDataType::kFloatingPoint; }

  /** Returns the endianness of the host. */
  constexpr Endian hostEndian()
  {
#ifdef WORDS_BIGENDIAN
    return Endian::kBig;
#else
    return Endian::kLittle;
#endif
  }

  /** Returns the byte size of a frame with a given sample specification. */
  size_t frameSize(StreamSpec spec);

  /** Calculates number of frames required for a given time. */
  size_t usecsToFrames(microseconds usecs, const StreamSpec& spec);

  /** Calculates time a given number of frames will take to play. */
  microseconds framesToUsecs(size_t frames, const StreamSpec& spec);

  /** Calculates number of bytes that are required for a given time */
  size_t usecsToBytes(microseconds usecs, const StreamSpec& spec);

  /** Returns the play time for a given number of bytes. */
  microseconds bytesToUsecs(size_t bytes, const StreamSpec& spec);

  /**
   * A ChannelMap describes an index-based mapping between the audio channels
   * of two audio streams or buffers. Use negative values to ignore a channel
   * in the operation.
   * If the channel map has less entries than the number of channels of the
   * source entity, the remaining channels are mapped to the taget channels
   * with the same index.
   */
  using ChannelMap = std::vector<int>;

  class Decibel {
  public:
    constexpr explicit Decibel(double count = 0.0f) : cnt_{count}
      { /* nothing */ }

    constexpr double value() const
      { return cnt_; }
    double amplitude() const
      { return std::pow(10.0f, cnt_ / 20.0f); }
    double power() const
      { return std::pow(10.0f, cnt_ / 10.0f); }
    constexpr Decibel operator+() const
      { return Decibel(*this); }
    constexpr Decibel operator-() const
      { return Decibel(-cnt_); }
    constexpr Decibel& operator+=(const Decibel& other)
      { cnt_ += other.value(); return *this; }
    constexpr Decibel& operator-=(const Decibel& other)
      { cnt_ -= other.value(); return *this; }
    template<typename T>
    constexpr Decibel& operator*=(const T& value)
      { cnt_ *= value; return *this; }
    template<typename T>
    constexpr Decibel& operator/=(const T& value)
      { cnt_ /= value; return *this; }

  private:
    double cnt_;
  };

  constexpr Decibel operator+(const Decibel& lhs, const Decibel& rhs)
  { return Decibel(lhs.value() + rhs.value()); }
  constexpr Decibel operator-(const Decibel& lhs, const Decibel& rhs)
  { return Decibel(lhs.value() - rhs.value()); }

  template<typename T>
  constexpr Decibel operator*(const Decibel& lhs, const T& value)
  { return Decibel(lhs.value() * value); }
  template<typename T>
  constexpr Decibel operator/(const Decibel& lhs, const T& value)
  { return Decibel(lhs.value() / value); }

  constexpr bool operator==(const Decibel& lhs, const Decibel& rhs)
  { return lhs.value() == rhs.value(); }
  constexpr bool operator!=(const Decibel& lhs, const Decibel& rhs)
  { return !(lhs == rhs); }
  constexpr bool operator<(const Decibel& lhs, const Decibel& rhs)
  { return lhs.value() < rhs.value(); }
  constexpr bool operator<=(const Decibel& lhs, const Decibel& rhs)
  { return lhs < rhs || lhs == rhs; }
  constexpr bool operator>(const Decibel& lhs, const Decibel& rhs)
  { return !(lhs <=  rhs); }
  constexpr bool operator>=(const Decibel& lhs, const Decibel& rhs)
  { return lhs > rhs || lhs == rhs; }

  constexpr Decibel operator "" _dB(unsigned long long value)
  { return Decibel(value); }
  constexpr Decibel operator "" _dB(long double value)
  { return Decibel(value); }

  constexpr double kMinVolume = 0.0;  // percent
  constexpr double kMaxVolume = 100.0;

  /*
   * Type of mapping from volume (in percent) to amplitude ratio [0,1]
   * https://lists.linuxaudio.org/archives/linux-audio-dev/2009-May/022198.html
   * https://www.dr-lex.be/info-stuff/volumecontrols.html
   */
  enum class VolumeMapping {
    kLinear     = 1,
    kQuadratic  = 2,
    kCubic      = 3
  };

  inline
  double amplitudeToVolume(double amp, VolumeMapping map = VolumeMapping::kCubic)
  {
    switch (map) {
    case VolumeMapping::kQuadratic:
      amp = std::sqrt(amp);
      break;
    case VolumeMapping::kCubic:
      amp = std::cbrt(amp);
      break;
    case VolumeMapping::kLinear:
      [[fallthrough]];
    default:
      /* linear mapping */
      break;
    };
    return std::clamp(amp * kMaxVolume, kMinVolume, kMaxVolume);
  }

  inline
  double volumeToAmplitude(double vol, VolumeMapping map = VolumeMapping::kCubic)
  {
    vol = vol / 100.0;

    switch (map) {
    case VolumeMapping::kQuadratic:
      vol = vol * vol;
      break;
    case VolumeMapping::kCubic:
      vol = vol * vol * vol;
      break;
    case VolumeMapping::kLinear:
      [[fallthrough]];
    default:
      /* nothing to do */
      break;
    };

    return vol;
  }

  inline Decibel amplitudeToDecibel(double amp)
  { return Decibel { 20.0 * std::log10(amp) }; }

  inline double decibelToAmplitude(const Decibel& dec)
  { return dec.amplitude(); }

  inline
  Decibel volumeToDecibel(double vol, VolumeMapping map = VolumeMapping::kCubic)
  { return amplitudeToDecibel(volumeToAmplitude(vol, map)); }

  inline
  double decibelToVolume(const Decibel& dec, VolumeMapping map = VolumeMapping::kCubic)
  { return amplitudeToVolume(decibelToAmplitude(dec), map); }

}//namespace audio
#endif//GMetronome_Audio_h
