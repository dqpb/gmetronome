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

#ifndef GMetronome_Filter_h
#define GMetronome_Filter_h

#include "Audio.h"
#include "AudioBuffer.h"

#include <vector>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <functional>
#include <cmath>

namespace audio {
namespace filter {

  constexpr SampleFormat defaultSampleFormat()
  {
    if (hostEndian() == Endian::kLittle)
      return SampleFormat::kFloat32LE;
    else
      return SampleFormat::kFloat32BE;
  }

  struct AutomationPoint
  {
    microseconds time;
    float value;
  };

  // compare helpers
  inline bool autoPointTimeLess(const AutomationPoint& lhs, const AutomationPoint& rhs)
  { return lhs.time < rhs.time; };
  inline bool autoPointValueLess(const AutomationPoint& lhs, const AutomationPoint& rhs)
  { return lhs.value < rhs.value; };

  using Automation = std::vector<AutomationPoint>;

  enum class Waveform
  {
    kSine,
    kTriangle,
    kSawtooth,
    kSquare
  };

  struct Oscillator
  {
    float frequency;
    float amplitude;
    Waveform shape;
  };

  template<typename PipeHead, typename FilterType>
  class FilterPipe {
  public:
    FilterPipe(PipeHead head, FilterType filter)
      : head_{std::move(head)}, filter_{std::move(filter)}
      {}
    template<typename DataType> void operator()(DataType& data)
      {
        head_(data);
        filter_(data);
      }
    template<typename Other> auto operator | (Other filter) &
      { return FilterPipe<FilterPipe, Other>(*this, std::move(filter)); }

    template<typename Other> auto operator | (Other filter) &&
      { return FilterPipe<FilterPipe, Other>(std::move(*this), std::move(filter)); }

  private:
    PipeHead head_;
    FilterType filter_;
  };

  template<typename Callable>
  class Filter {
  public:
    Filter(Callable fu) : fu_{std::move(fu)}
      {}
    template<typename...Args>
    Filter(Args...args) : fu_{std::forward<Args>(args)...}
      {}
    template<typename DataType> void operator()(DataType& data)
      { std::invoke(fu_, data); }

    template<typename Other> auto operator | (Other filter) &
      { return FilterPipe(*this, std::move(filter)); }

    template<typename Other> auto operator | (Other filter) &&
      { return FilterPipe(std::move(*this), std::move(filter)); }

  private:
    Callable fu_;
  };

  /**
   * @class Gain
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Gain {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Gain(double gain_l, double gain_r)
      : gain_l_{static_cast<float>(gain_l)}, gain_r_{static_cast<float>(gain_r)}
      { /* nothing */ }
    explicit Gain(double gain) : Gain(gain, gain)
      { /* nothing */ }
    explicit Gain(const Automation& points) : gain_l_{1}, gain_r_{1}, points_(points)
      {
        assert(std::is_sorted(points_.begin(), points_.end(), autoPointTimeLess));
      }
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate > 0);

        if (buffer.empty())
          return;

        if (points_.empty())
        {
          auto frames = viewFrames<Format>(buffer);
          for (auto& frame : frames)
            frame *= {gain_l_, gain_r_};
        }
        else
          applyAutomation(buffer);
      }

    void applyAutomation(ByteBuffer& buffer)
      {
        auto frames = viewFrames<Format>(buffer);
        size_t n_frames = frames.size();

        float cur_value = points_.front().value;
        size_t cur_frame = 0;

        for (const auto& pt : points_)
        {
          size_t pt_frame = usecsToFrames(pt.time, buffer.spec());
          float delta_value = pt.value - cur_value;
          float delta_frames = pt_frame - cur_frame + 1;

          if (delta_frames > 0)
          {
            float slope = delta_value / delta_frames;
            size_t end_frame = std::min(pt_frame + 1, n_frames);
            for (; cur_frame < end_frame; ++cur_frame)
            {
              cur_value += slope;
              frames[cur_frame] *= cur_value;
            }
          }
        }
        for (; cur_frame < n_frames; ++cur_frame)
          frames[cur_frame] *= cur_value;
      }
  private:
    float gain_l_;
    float gain_r_;
    Automation points_;
  };

  /**
   * @class Noise
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Noise {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    explicit Noise(double amplitude) : amp_{static_cast<float>(amplitude)} {}

    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.format() == Format);

        auto frames = viewFrames<Format>(buffer);
        for (auto& frame : frames)
        {
          frame += {
            amp_ * uniform_distribution(),
            amp_ * uniform_distribution()
          };
        }
      }
  private:
    float amp_;

    float uniform_distribution()
      {
        static std::uint32_t value = static_cast<std::uint32_t>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count());

        value = value * 214013 + 2531011;
        return 2.0f * (((value & 0x3FFFFFFF) >> 15) / 32767.0) - 1.0f;
      }
  };

  /**
   * @class Sine
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Sine {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Sine(double frequency, double amplitude)
      : freq_{static_cast<float>(frequency)}, amp_{static_cast<float>(amplitude)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));

        constexpr float kPi = M_PI;
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr float kPiHalf = kPi / 2.0f;
        constexpr float kThreePiHalf = 3.0f * kPi / 2.0f;

        auto frames = viewFrames<Format>(buffer);
        float omega = kTwoPi * freq_ / buffer.spec().rate;
        float arg = -kPiHalf;

        for (auto& frame : frames)
        {
          if (arg > kPiHalf)
            frame -= amp_ * bhaskaraCos(arg - kPi);
          else
            frame += amp_ * bhaskaraCos(arg);

          arg += omega;
          if (arg > kThreePiHalf)
            arg = arg - kTwoPi;
        }
      }
  private:
    float freq_;
    float amp_;

    // range: (-pi,pi)
    float chebyshevSin(float x)
      {
        static const std::array<float,6> coeffs =
          {
            -0.10132118f,          // x
             0.0066208798f,        // x^3
            -0.00017350505f,       // x^5
             0.0000025222919f,     // x^7
            -0.000000023317787f,   // x^9
             0.00000000013291342f, // x^11
          };

        static const float pi_major = 3.1415927f;
        static const float pi_minor = -0.00000008742278f;

        float x2  = x*x;
        float p11 = coeffs[5];
        float p9  = p11*x2 + coeffs[4];
        float p7  = p9*x2  + coeffs[3];
        float p5  = p7*x2  + coeffs[2];
        float p3  = p5*x2  + coeffs[1];
        float p1  = p3*x2  + coeffs[0];

        return (x - pi_major - pi_minor) *
          (x + pi_major + pi_minor) * p1 * x;
      }

    // range: (-pi/2,pi/2)
    float bhaskaraCos(float x)
      {
        static constexpr float kPi2 = M_PI * M_PI;
        float x2 = x * x;
        return (kPi2 - 4*x2) / (kPi2 + x2);
      }
  };

  /**
   * @class Triangle
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Triangle {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Triangle(double frequency, double amplitude)
      : freq_{static_cast<float>(frequency)}, amp_{static_cast<float>(amplitude)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          // TODO: too slow
          float triangle = 2.0 * std::asin(std::sin(omega * frame_index)) / M_PI;
          frames[frame_index] += amp_ * triangle;
        }
      }
  private:
    float freq_;
    float amp_;
  };

  /**
   * @class Sawtooth
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Sawtooth {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Sawtooth(float frequency, float amplitude) : freq_{frequency}, amp_{amplitude}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          float time = (omega * frame_index) / (2.0 * M_PI);
          float sawtooth = 2.0 * (time - std::floor(time) - 0.5);
          frames[frame_index] += amp_ * sawtooth;
        }
      }
  private:
    float freq_;
    float amp_;
  };

  /**
   * @class Square
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Square {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Square(double frequency, double amplitude)
      : freq_{static_cast<float>(frequency)}, amp_{static_cast<float>(amplitude)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          float sine = std::sin(omega * frame_index); // TODO: too slow
          if (sine >= 0)
            frames[frame_index] += amp_;
          else
            frames[frame_index] -= amp_;
        }
      }
  private:
    float freq_;
    float amp_;
  };

  /**
   * @class Osc
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Osc {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    explicit Osc(const std::vector<Oscillator>& oscs) : oscs_{oscs}
      {/*nothing*/}

    Osc(float frequency, float amplitude, Waveform form)
      : oscs_{{frequency, amplitude, form}}
      {/*nothing*/}

    void operator()(ByteBuffer& buffer)
      {
        for (const auto& osc : oscs_)
        {
          switch (osc.shape) {
          case Waveform::kSine:
            Sine(osc.frequency, osc.amplitude)(buffer);
            break;
          case Waveform::kTriangle:
            Triangle(osc.frequency, osc.amplitude)(buffer);
            break;
          case Waveform::kSawtooth:
            Sawtooth(osc.frequency, osc.amplitude)(buffer);
            break;
          case Waveform::kSquare:
            Square(osc.frequency, osc.amplitude)(buffer);
            break;
          }
        }
      }
  private:
    const std::vector<Oscillator> oscs_;
  };

  /**
   * @class Normalize
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Normalize {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Normalize(float gain_l, float gain_r) : gain_l_{gain_l}, gain_r_{gain_r}
      {/*nothing*/}
    explicit Normalize(float gain) : Normalize(gain, gain)
      {/*nothing*/}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().channels == 2);
        auto frames = viewFrames<Format>(buffer);
        float max = 0.0;
        for (auto& frame : frames)
          max = std::max(max, std::max(std::abs(frame[0]), std::abs(frame[1])));

        for (auto& frame : frames)
          frame *= { gain_l_ / max, gain_r_ / max};
      }
  private:
    float gain_l_;
    float gain_r_;
  };

  /**
   * @class Mix
   *
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Mix {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Mix(const ByteBuffer& buffer) : buffer_{buffer}
      {/*nothing*/}

    void operator()(ByteBuffer& buffer)
      {
        auto frames1 = viewFrames<Format>(buffer);
        auto frames2 = viewFrames<Format>(buffer_);

        auto it2 = frames2.begin();
        for (auto it1 = frames1.begin();
             it1 != frames1.end() && it2 != frames2.end();
             ++it1, ++it2)
        {
          *it1 += *it2;
        }
      }
  private:
    const ByteBuffer& buffer_;
  };

  /**
   * @class Smooth
   */
  template<SampleFormat Format = defaultSampleFormat()>
  class Smooth {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

    using Frame = FrameView<Format, ByteBuffer::pointer>;
    using Frames = FrameContainerView<Format, ByteBuffer::pointer>;

  public:
    explicit Smooth(const Automation& points) : points_{points}, cache_frames_{0}, channels_{0}
      {
        assert(std::is_sorted(points_.begin(), points_.end(), autoPointTimeLess));

        auto it = std::max_element(points_.begin(), points_.end(), autoPointValueLess);
        cache_.reserve(std::max<size_t>(0, it->value) + 1);
      }

    explicit Smooth(size_t kernel_width)
      : points_{{{0ms}, {(float)kernel_width}}}, cache_frames_{0}, channels_{0}
      { /* nothing */ }

    void operator()(ByteBuffer& buffer)
      {
        if (points_.empty() || buffer.empty())
          return;

        channels_ = buffer.channels();

        auto frames = viewFrames<Format>(buffer);

        float cur_value = points_.front().value;
        size_t cur_frame = 0;
        size_t n_frames = buffer.frames();

        for (const auto& pt : points_)
        {
          size_t pt_frame = usecsToFrames(pt.time, buffer.spec());
          float delta_value = pt.value - cur_value;
          float delta_frames = pt_frame - cur_frame + 1;

          // if (delta_frames > 0) // this is always true
          {
            float slope = delta_value / delta_frames;
            size_t end_frame = std::min(pt_frame + 1, n_frames);
            for (; cur_frame < end_frame; ++cur_frame)
            {
              cur_value += slope;
              processFrame(frames, cur_frame, cur_value);
            }
          }
        }
        for (; cur_frame < n_frames; ++cur_frame)
          processFrame(frames, cur_frame, cur_value);
      }

    void processFrame(Frames& frames, size_t frame_index, size_t kernel_size)
      {
        Frame& frame = frames[frame_index];

        pushFrame(frame);

        if (cache_frames_ - 1 > kernel_size)
          popFrames(2);
        else if (cache_frames_ > kernel_size)
          popFrames(1);

        if (cache_frames_ > 1)
          for (size_t channel = 0; channel < frame.size(); ++channel)
            frame[channel] = average(channel);
      }

    void pushFrame(const Frame& frame)
      {
        cache_.insert(cache_.end(), frame.begin(), frame.end());
        ++cache_frames_;
      }

    void popFrames(size_t n = 1)
      {
        cache_.erase(cache_.begin(), cache_.begin() + n * channels_);
        cache_frames_ -= n;
      }

    float average(int channel)
      {
        if (cache_frames_ == 0)
          return 0.0;

        float sum = 0.0;
        for (auto it = cache_.begin() + channel; it < cache_.end(); it += channels_)
          sum += *it;

        return sum / cache_frames_;
      }

  private:
    Automation points_;
    std::vector<float> cache_;
    size_t cache_frames_;
    size_t channels_;
  };

  namespace std {

    // some shortcuts for default filters
    using Gain      = Filter<Gain<defaultSampleFormat()>>;
    using Noise     = Filter<Noise<defaultSampleFormat()>>;
    using Sine      = Filter<Sine<defaultSampleFormat()>>;
    using Triangle  = Filter<Triangle<defaultSampleFormat()>>;
    using Sawtooth  = Filter<Sawtooth<defaultSampleFormat()>>;
    using Square    = Filter<Square<defaultSampleFormat()>>;
    using Osc       = Filter<Osc<defaultSampleFormat()>>;
    using Normalize = Filter<Normalize<defaultSampleFormat()>>;
    using Mix       = Filter<Mix<defaultSampleFormat()>>;
    using Smooth    = Filter<Smooth<defaultSampleFormat()>>;

  }//namespace std

} //namespace filter
}//namespace audio
#endif//GMetronome_Filter_h
