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

  constexpr SampleFormat kDefaultSampleFormat =
    (hostEndian() == Endian::kLittle) ? SampleFormat::kFloat32LE : SampleFormat::kFloat32BE;

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
    Filter(Args&&...args) : fu_{std::forward<Args>(args)...}
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
   * @class Zero
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Zero {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Zero() { /* nothing */ }

    void operator()(ByteBuffer& buffer)
      {
        std::fill(buffer.begin(), buffer.end(), 0);
      }
  };

  /**
   * @class Gain
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
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
  template<SampleFormat Format = kDefaultSampleFormat>
  class Noise {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    explicit Noise(double amplitude) : amp_{static_cast<float>(amplitude)} {}

    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.format() == Format);

        if (amp_ == 0.0f)
          return;

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
  template<SampleFormat Format = kDefaultSampleFormat>
  class Sine {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Sine(double frequency, double amplitude, double detune = 0.0)
      : freq_{static_cast<float>(frequency)},
        amp_{static_cast<float>(amplitude)},
        detune_{static_cast<float>(detune)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));
        assert(buffer.channels() == 2);

        if (amp_ == 0.0f)
          return;

        constexpr float kPi = M_PI;
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr float kPiHalf = kPi / 2.0f;
        constexpr float kThreePiHalf = 3.0f * kPi / 2.0f;
        constexpr int kVoices = 4;

        float amp = amp_ / 2; // two voices per channel

        auto frames = viewFrames<Format>(buffer);

        float omega[kVoices] = {
          kTwoPi * (freq_ - detune_ / 4) / buffer.spec().rate,
          kTwoPi * (freq_ + detune_ / 4) / buffer.spec().rate,
          kTwoPi * (freq_ + detune_ / 2) / buffer.spec().rate,
          kTwoPi * (freq_ - detune_ / 2) / buffer.spec().rate
        };

        float arg[kVoices] = {
          -kPiHalf,
          -kPiHalf,
          -kPiHalf,
          -kPiHalf
        };

        for (auto& frame : frames)
        {
          for (int voice = 0; voice < kVoices; ++voice)
          {
            int channel = voice % 2;

            if (arg[voice] > kPiHalf)
              frame[channel] -= amp * bhaskaraCos(arg[voice] - kPi);
            else
              frame[channel] += amp * bhaskaraCos(arg[voice]);

            arg[voice] += omega[voice];
            if (arg[voice] > kThreePiHalf)
              arg[voice] = arg[voice] - kTwoPi;
          }
        }
      }
  private:
    float freq_;
    float amp_;
    float detune_;

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
  template<SampleFormat Format = kDefaultSampleFormat>
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

        if (amp_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          // TODO: too slow
          frames[frame_index] += (amp_ * 8.0 / (M_PI * M_PI)) * ( + (1.0 /  1.0) * std::sin(1.0 * omega * frame_index)
                                                                  - (1.0 /  9.0) * std::sin(3.0 * omega * frame_index)
                                                                  + (1.0 / 25.0) * std::sin(5.0 * omega * frame_index)
                                                                  - (1.0 / 49.0) * std::sin(7.0 * omega * frame_index)
                                                                  + (1.0 / 81.0) * std::sin(9.0 * omega * frame_index) );
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
  template<SampleFormat Format = kDefaultSampleFormat>
  class Sawtooth {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Sawtooth(double frequency, double amplitude)
      : freq_{static_cast<float>(frequency)}, amp_{static_cast<float>(amplitude)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));

        if (amp_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          // TODO: too slow
          frames[frame_index] += - (amp_ * 2.0 / M_PI) * ( - std::sin(omega * frame_index)
                                                           + (1.0 / 2.0) * std::sin(2.0 * omega * frame_index)
                                                           - (1.0 / 3.0) * std::sin(3.0 * omega * frame_index)
                                                           + (1.0 / 4.0) * std::sin(4.0 * omega * frame_index)
                                                           - (1.0 / 5.0) * std::sin(5.0 * omega * frame_index)
                                                           + (1.0 / 6.0) * std::sin(6.0 * omega * frame_index)
                                                           - (1.0 / 7.0) * std::sin(7.0 * omega * frame_index)
                                                           + (1.0 / 8.0) * std::sin(8.0 * omega * frame_index)
                                                           - (1.0 / 9.0) * std::sin(9.0 * omega * frame_index) );

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
  template<SampleFormat Format = kDefaultSampleFormat>
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

        if (amp_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);
        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          // TODO: too slow
          frames[frame_index] += amp_ * (4.0 / M_PI) * ( std::sin(omega * frame_index)
                                                        + (1.0 / 3.0) * std::sin(3.0 * omega * frame_index)
                                                        + (1.0 / 5.0) * std::sin(5.0 * omega * frame_index)
                                                        + (1.0 / 7.0) * std::sin(7.0 * omega * frame_index)
                                                        + (1.0 / 9.0) * std::sin(9.0 * omega * frame_index) );
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
  template<SampleFormat Format = kDefaultSampleFormat>
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
   * @class Ring
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Ring {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types at the moment");

  public:
    Ring(double freq)
      : freq_{static_cast<float>(freq)}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));
        assert(buffer.channels() == 2);

        if (freq_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);

        float omega = 2.0 * M_PI * freq_ / buffer.spec().rate;
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          float sine = std::sin(omega * frame_index); // TODO: too slow
          frames[frame_index] *= sine;
        }
      }
  private:
    float freq_;
  };

  /**
   * @class Normalize
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
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
  template<SampleFormat Format = kDefaultSampleFormat>
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
  template<SampleFormat Format = kDefaultSampleFormat>
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

          if (delta_frames > 0)
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
    using Zero      = Filter<Zero<kDefaultSampleFormat>>;
    using Gain      = Filter<Gain<kDefaultSampleFormat>>;
    using Noise     = Filter<Noise<kDefaultSampleFormat>>;
    using Sine      = Filter<Sine<kDefaultSampleFormat>>;
    using Triangle  = Filter<Triangle<kDefaultSampleFormat>>;
    using Sawtooth  = Filter<Sawtooth<kDefaultSampleFormat>>;
    using Square    = Filter<Square<kDefaultSampleFormat>>;
    using Osc       = Filter<Osc<kDefaultSampleFormat>>;
    using Ring      = Filter<Ring<kDefaultSampleFormat>>;
    using Normalize = Filter<Normalize<kDefaultSampleFormat>>;
    using Mix       = Filter<Mix<kDefaultSampleFormat>>;
    using Smooth    = Filter<Smooth<kDefaultSampleFormat>>;

  }//namespace std

} //namespace filter
}//namespace audio
#endif//GMetronome_Filter_h
