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
#include "Wavetable.h"

#include <vector>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <cmath>

namespace audio {
namespace filter {

  constexpr SampleFormat kDefaultSampleFormat =
    (hostEndian() == Endian::kLittle) ? SampleFormat::kFloat32LE : SampleFormat::kFloat32BE;

  class Automation {
  public:
    struct Point { microseconds time; float value; };

    Automation()
      { /* nothing */ }
    Automation(std::initializer_list<Point> list)
      : points_{std::move(list)},
        r_cache_{points_.begin()}
      { /* nothing */ }

    const std::vector<Point>& points() const
      { return points_; }

    float lookup(const microseconds& time) const
      {
        if (points_.empty())
          return 0.0f;

        // if (r_cache_ == points_.begin())
        // {

        // }
        // else if (r_cache_ == points_.end())
        // {
        // }
        // else
        // {
        // }

        // if (r_cache_ != points_.end())
        // {

        // }

        // if (r_cache_ == points_.begin())
        // {
        // }
        // else if (r_cache_ != points_.end())
        // {
        //   if (r_cache_->time >= time)
        //   {
        //     if ()
        //     {}
        //   }

        // }

        auto r = std::lower_bound(points_.begin(), points_.end(), time,
                                  [] (const auto& lhs, const auto& value)
                                    { return lhs.time < value; });

        if (r == points_.end())
        {
          return points_.back().value;
        }
        else if (r == points_.begin())
        {
          return points_.front().value;
        }
        else
        {
          auto l = std::prev(r);
          float slope = (r->value - l->value) / (r->time - l->time).count();
          return l->value + (slope * (time - l->time).count());
        }
      }

    bool empty() const
      { return points_.empty(); }

  private:
    std::vector<Point> points_;
    std::vector<Point>::const_iterator r_cache_;
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
      "this filter only supports floating point types");

  public:
    Gain(double gain_l, double gain_r)
      : gain_l_{static_cast<float>(gain_l)}, gain_r_{static_cast<float>(gain_r)}
      { /* nothing */ }
    explicit Gain(double gain) : Gain(gain, gain)
      { /* nothing */ }
    explicit Gain(const Automation& envelope) : gain_l_{1}, gain_r_{1}, envelope_(envelope)
      { /* nothing */ }
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate > 0);

        if (buffer.empty())
          return;

        auto frames = viewFrames<Format>(buffer);
        if (envelope_.empty())
        {
          for (auto& frame : frames)
            frame *= {gain_l_, gain_r_};
        }
        else
        {
          double frame_duration = 1000000.0 / buffer.rate(); //usecs
          for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
          {
            microseconds frame_time {static_cast<int>(frame_index * frame_duration)};
            frames[frame_index] *= envelope_.lookup(frame_time);
          }
        }
      }
  private:
    float gain_l_;
    float gain_r_;
    Automation envelope_;
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
   * @class Wave
   * @brief A wavetable oscillator
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Wave {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    Wave(const Wavetable& tbl, float frequency, float amplitude = 1.0,
         float phase = 0.0, float detune = 0.0)
      : tbl_{tbl},
        freq_{frequency},
        amp_{amplitude},
        phase_{phase},
        detune_{frequency * std::pow(2.0f, detune / 1200.0f) - frequency}
      {}
    void operator()(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));
        assert(buffer.channels() == 2);

        if (amp_ == 0.0f || freq_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);
        float delta_t = 1.0 / buffer.spec().rate;
        float phase_t = phase_ / (2.0 * M_PI) / freq_;
        auto& tbl_page = tbl_.lookup(freq_);

        if (detune_ != 0.0f)
        {
          float amp_half = amp_ / 2.0f;
          float detune_half = detune_ / 2.0f;

          for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
          {
            float time = delta_t * frame_index + phase_t;
            frames[frame_index] += {
              amp_half * (tbl_page.lookup(freq_ - detune_,     time) +
                          tbl_page.lookup(freq_ + detune_half, time)),
              amp_half * (tbl_page.lookup(freq_ + detune_,     time) +
                          tbl_page.lookup(freq_ - detune_half, time))
            };
          }
        }
        else
        {
          for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
          {
            float time = delta_t * frame_index + phase_t;
            frames[frame_index] += amp_ * tbl_page.lookup(freq_, time);
          }
        }
      }
  private:
    const Wavetable& tbl_;
    float freq_;
    float amp_;
    float phase_;
    float detune_;
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
        float max = 0.0f;
        for (auto& frame : frames)
          max = std::max(max, std::max(std::abs(frame[0]), std::abs(frame[1])));

        if (max != 0.0f)
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
      "this filter only supports floating point types");

    using Frame = FrameView<Format, ByteBuffer::pointer>;
    using Frames = FrameContainerView<Format, ByteBuffer::pointer>;

  public:
    explicit Smooth(const Automation& kernel_width)
      : kernel_width_{kernel_width},
        cache_frames_{0},
        channels_{0}
      {
        if (auto it = std::max_element(kernel_width_.points().begin(),
                                       kernel_width_.points().end(),
                                       [] (const auto& lhs, const auto& rhs)
                                         { return lhs.value < rhs.value;});
            it != kernel_width_.points().end())
        {
          cache_.reserve(std::max<size_t>(0, it->value) + 1);
        }
        else
          cache_.reserve(1);
      }

    explicit Smooth(size_t kernel_width)
      : kernel_width_{{{0ms}, (float) kernel_width}}, cache_frames_{0}, channels_{0}
      { /* nothing */ }

    void operator()(ByteBuffer& buffer)
      {
        if (kernel_width_.empty() || buffer.empty())
          return;

        channels_ = buffer.channels();

        auto frames = viewFrames<Format>(buffer);

        double frame_duration = 1000000.0 / buffer.rate(); //usecs
        for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
        {
          microseconds frame_time {static_cast<int>(frame_index * frame_duration)};
          processFrame(frames, frame_index, kernel_width_.lookup(frame_time));
        }
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
    Automation kernel_width_;
    std::vector<float> cache_;
    size_t cache_frames_;
    size_t channels_;
  };

  namespace std {

    // some shortcuts for default filters
    using Zero      = Filter<Zero<kDefaultSampleFormat>>;
    using Gain      = Filter<Gain<kDefaultSampleFormat>>;
    using Noise     = Filter<Noise<kDefaultSampleFormat>>;
    using Wave      = Filter<Wave<kDefaultSampleFormat>>;
    using Ring      = Filter<Ring<kDefaultSampleFormat>>;
    using Normalize = Filter<Normalize<kDefaultSampleFormat>>;
    using Mix       = Filter<Mix<kDefaultSampleFormat>>;
    using Smooth    = Filter<Smooth<kDefaultSampleFormat>>;

  }//namespace std

} //namespace filter
}//namespace audio
#endif//GMetronome_Filter_h
