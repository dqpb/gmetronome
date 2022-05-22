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
#include <type_traits>

namespace audio {
namespace filter {

  template<typename PipeHead, typename FilterType>
  class FilterPipe {
  public:
    FilterPipe()
      { /* nothing */ }
    FilterPipe(PipeHead head, FilterType filter)
      : head_{std::move(head)}, filter_{std::move(filter)}
      { /* nothing */ }
    void prepare(const StreamSpec& spec)
      { head_.prepare(spec); filter_.prepare(spec); }

    template<typename DataType> void process(DataType& data)
      { head_.process(data); filter_.process(data); }

    template<typename DataType> void operator()(DataType& data)
      { process(data); }

    template<typename Other> auto operator | (Other filter) &
      { return FilterPipe<FilterPipe, Other>(*this, std::move(filter)); }

    template<typename Other> auto operator | (Other filter) &&
      { return FilterPipe<FilterPipe, Other>(std::move(*this), std::move(filter)); }

  private:
    PipeHead head_;
    FilterType filter_;

    template<std::size_t I, typename H, typename F>
    friend constexpr auto& get(FilterPipe<H,F>& pipe) noexcept;

    template<typename T, typename H, typename F>
    friend constexpr T& get(FilterPipe<H,F>& pipe) noexcept;
  };

  template<typename Callable>
  class Filter : public Callable {
  public:
    template<typename...Args>
    Filter(Args&&...args) : Callable(std::forward<Args>(args)...)
      { /* nothing */ }
    void prepare(const StreamSpec& spec)
      { Callable::prepare(spec); }

    template<typename DataType> void process(DataType& data)
      { Callable::process(data); }

    template<typename DataType> void operator()(DataType& data)
      { process(data); }

    template<typename Other> auto operator | (Other filter) &
      { return FilterPipe(*this, std::move(filter)); }

    template<typename Other> auto operator | (Other filter) &&
      { return FilterPipe(std::move(*this), std::move(filter)); }
  };

  /**
   * @struct FilterPipeSize
   * @brief Compile-time access to the number of filters in the pipe
   */
  template<typename P> struct FilterPipeSize {};

  template<typename PipeHead, typename FilterType>
  struct FilterPipeSize<FilterPipe<PipeHead, FilterType>>
  {
    static constexpr std::size_t value = FilterPipeSize<PipeHead>::value + 1;
  };

  template<typename Callable>
  struct FilterPipeSize<Filter<Callable>>
  {
    static constexpr std::size_t value = 1;
  };

  /**
   * @struct FilterPipeElement
   * @brief Compile-time indexed access to the types of the filters in the pipe
   */
  template<std::size_t I, typename P, typename Enable = void>
  struct FilterPipeElement {};

  template<typename Callable>
  struct FilterPipeElement<0, Filter<Callable>>
  {
    using type = Filter<Callable>;
  };

  template<std::size_t I, typename PipeHead, typename FilterType>
  struct FilterPipeElement<I, FilterPipe<PipeHead, FilterType>,
                           std::enable_if_t<(I < FilterPipeSize<PipeHead>::value)>>
  {
    using type = typename FilterPipeElement<I,PipeHead>::type;
  };

  template<std::size_t I, typename PipeHead, typename FilterType>
  struct FilterPipeElement<I, FilterPipe<PipeHead, FilterType>,
                           std::enable_if_t<(I == FilterPipeSize<PipeHead>::value)>>
  {
    using type = FilterType;
  };

  /**
   * @function get
   * @brief Extracts the Ith filter from the pipe.
   */
  template<std::size_t I, typename PipeHead, typename FilterType>
  constexpr auto& get(FilterPipe<PipeHead, FilterType>& pipe) noexcept
  {
    static_assert(I < FilterPipeSize<FilterPipe<PipeHead, FilterType>>::value);

    if constexpr (I == FilterPipeSize<PipeHead>::value)
      return pipe.filter_;
    else
      return get<I>(pipe.head_);
  }

  template<std::size_t I, typename Callable>
  constexpr Filter<Callable>& get(Filter<Callable>& pipe) noexcept
  {
    static_assert(I == 0);
    return pipe;
  }

  /**
   * @function get
   * @brief Extracts the element from the pipe whose type is T.
   *
   * If the pipe contains more than one element of type T, the last
   * element is returned.
   */
  template<typename T, typename PipeHead, typename FilterType>
  constexpr T& get(FilterPipe<PipeHead, FilterType>& pipe) noexcept
  {
    if constexpr (std::is_same<T, FilterType>::value)
      return pipe.filter_;
    else
      return get<T>(pipe.head_);
  }

  template<typename T, typename Callable>
  constexpr T& get(Filter<Callable>& pipe) noexcept
  {
    static_assert( std::is_same<T,Filter<Callable>>::value );
    return pipe;
  }


  constexpr SampleFormat kDefaultSampleFormat =
    (hostEndian() == Endian::kLittle) ? SampleFormat::kFloat32LE : SampleFormat::kFloat32BE;

  using seconds_dbl = std::chrono::duration<double>;

  class Automation {
  public:
    struct Point { seconds_dbl time; float value; };

    using PointContainer = std::vector<Point>;
    using Iterator = PointContainer::iterator;
    using ConstIterator = PointContainer::const_iterator;

    Automation()
      { /* nothing */ }
    Automation(std::initializer_list<Point> list) : points_{std::move(list)}
      { sort(); }

    const PointContainer& points() const
      { return points_; }

    template<typename Iterator, typename Function>
    void apply(Iterator begin,
               Iterator end,
               const seconds_dbl& start,
               const seconds_dbl& step,
               Function fu)
      {
        if (points_.empty())
          return;

        auto time = start;
        auto r = points_.begin();
        bool r_changed = false;
        double value = points_.front().value;
        double value_step = 0.0f;

        for (auto it = begin; it != end; ++it, time+=step)
        {
          // search forward
          for (;r != points_.end() && r->time < time; ++r, r_changed=true);

          if (r_changed)
          {
            auto l = std::prev(r);
            if (r == points_.end())
            {
              value = l->value;
              value_step = 0.0;
            }
            else
            {
              double slope = (r->value - l->value) / (r->time - l->time).count();
              value = l->value + slope * (time - l->time).count();
              value_step = slope * step.count();
            }
            r_changed = false;
          }

          fu(*it, time, value);
          value += value_step;
        }
      }

    bool empty() const
      { return points_.empty(); }
    ConstIterator begin() const
      { return points_.begin(); }
    ConstIterator end() const
      { return points_.end(); }

    void insert(ConstIterator pos, std::initializer_list<Point> list)
      {
        points_.insert(pos, std::move(list));
        sort();
      }
    void append(std::initializer_list<Point> list)
      { insert(end(), std::move(list)); }
    void prepend(std::initializer_list<Point> list)
      { insert(begin(), std::move(list)); }

  private:
    PointContainer points_;

    void sort() {
      std::stable_sort(points_.begin(), points_.end(),
                       [] (const auto&  lhs, const auto& rhs) {
                         return lhs.time < rhs.time;
                       });
    }
  };

  /**
   * @class FIR
   * @brief Compute the convolution of an audio buffer and a filter kernel
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class FIR {
    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    explicit FIR(std::vector<float> kernel = {}) : kernel_{std::move(kernel)}
      {}

    void swapKernel(std::vector<float>& kernel)
      { kernel_.swap(kernel); }

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }

    void process(ByteBuffer& buffer)
      {
        auto channels = viewChannels<Format>(buffer);
        for (auto& channel : channels)
        {
          for(auto it = channel.rbegin(); it != channel.rend(); ++it)
          {
            float sum = 0.0f;
            auto s = it;
            for (auto& k : kernel_)
            {
              sum += k * (*s);
              if (++s == channel.rend())
                break;
            }
            *it = sum;
          }
        }
      }
  private:
    std::vector<float> kernel_;
  };

  /**
   * @class Lowpass
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Lowpass : private FIR<Format> {
    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    explicit Lowpass(float cutoff = 100.0f, size_t kernel_width = 31)
      : cutoff_{cutoff}, kernel_width_{kernel_width}
      { /* nothing */ }

    void setCutoff(float cutoff)
      {
        if (cutoff_ != cutoff )
        {
          cutoff_ = cutoff;
          need_rebuild_kernel_ = true;
        }
      }
    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );

        if (rate_ != spec.rate)
        {
          rate_ = spec.rate;
          need_rebuild_kernel_ = true;
        }
      }
    void process(ByteBuffer& buffer)
      {
        if (need_rebuild_kernel_)
          rebuildKernel();

        FIR<Format>::process(buffer);
      }

  private:
    float cutoff_;
    size_t kernel_width_;
    float rate_{44100.0};
    bool need_rebuild_kernel_{true};

    void rebuildKernel()
      {
        std::vector<float> kernel;
        FIR<Format>::swapKernel(kernel);

        if (kernel.size() != kernel_width_)
          kernel.resize(kernel_width_);

        std::size_t N = kernel.size();
        std::size_t M = N-1;
        double fc = std::clamp(cutoff_ / rate_, 0.0f, 0.5f);

        double sum = 0.0;
        for (std::size_t i = 0; i < N; ++i)
        {
          std::size_t I = i - M / 2;
          if (I == 0)
            kernel[i] = 2.0 * M_PI * fc;
          else
            kernel[i] = std::sin(2.0 * M_PI * fc * I) / I;

          // Blackman window
          kernel[i] *= 0.42
            - 0.50 * std::cos(2.0 * M_PI * i / M)
            + 0.08 * std::cos(4.0 * M_PI * i / M);

          sum += kernel[i];
        }
        // Normalize kernel
        for (auto& k : kernel)
          k /= sum;

        FIR<Format>::swapKernel(kernel);
        need_rebuild_kernel_ = false;
      }
  };

  /**
   * @class Zero
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Zero {
    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");
  public:
    Zero() { /* nothing */ }

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }
    void process(ByteBuffer& buffer)
      { std::fill(buffer.begin(), buffer.end(), 0); }
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
    explicit Gain(Automation envelope = {}) : envelope_(std::move(envelope))
      { /* nothing */ }

    void setEnvelope(Automation envelope)
      { envelope_ = std::move(envelope); }

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }
    void process(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate > 0);

        if (buffer.empty())
          return;

        auto frames = viewFrames<Format>(buffer);
        seconds_dbl frame_duration {1.0 / buffer.rate()};

        envelope_.apply(frames.begin(), frames.end(), 0ms, frame_duration,
                        [] (auto& frame, const auto& time, float value)
                          { frame *= value; });
      }
  private:
    Automation envelope_;
  };

  /**
   * @class Noise
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Noise {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    enum class Mode
    {
      kBlock,
      kContiguous
    };

  public:
    explicit Noise(float amp = 1.0f) : amp_{amp}
      { /* nothing */ }

    explicit Noise(const Decibel& level)
      : amp_{static_cast<float>(level.amplitude())}
      { /* nothing */ }

    void setLevel(const Decibel& level)
      { amp_ = static_cast<float>(level.amplitude()); }

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }

    void process(ByteBuffer& buffer)
      {
        assert(buffer.format() == Format);

        if (amp_ == 0.0f)
          return;

        if (mode_ == Mode::kBlock)
          value_ = seed_;

        auto frames = viewFrames<Format>(buffer);
        for (auto& frame : frames)
        {
          frame += {
            amp_ * uniform_distribution(),
            amp_ * uniform_distribution()
          };
        }
      }

    Mode mode() const
      { return mode_; }

    void setMode(Mode mode)
      { mode_ = mode; }

    void seed(std::uint32_t value = make_seed())
      { seed_ = value_ = value; }

    static std::uint32_t make_seed()
      {
        return  static_cast<std::uint32_t>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count());
      }
  private:
    float amp_;
    std::uint32_t seed_{0};
    std::uint32_t value_{0};
    Mode mode_{Mode::kBlock};

    float uniform_distribution()
      {
        value_ = value_ * 214013 + 2531011;
        return 2.0f * (((value_ & 0x3FFFFFFF) >> 15) / 32767.0) - 1.0f;
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
    Wave(const Wavetable& tbl, float freq, float amp = 0.0f,
         float phase = 0.0f, float detune = 0.0f)
      : tbl_{tbl},
        freq_{freq},
        amp_{amp},
        phase_{phase},
        detune_{freq * std::pow(2.0f, detune / 1200.0f) - freq}
      {}

    Wave(const Wavetable& tbl, float freq, const Decibel& level = 0_dB,
         float phase = 0.0f, float detune = 0.0f)
      : tbl_{tbl},
        freq_{freq},
        amp_{static_cast<float>(level.amplitude())},
        phase_{phase},
        detune_{freq * std::pow(2.0f, detune / 1200.0f) - freq}
      {}

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }

    void process(ByteBuffer& buffer)
      {
        assert(buffer.spec().rate != 0);
        assert(isFloatingPoint(buffer.spec().format));
        assert(buffer.channels() == 2);

        if (amp_ == 0.0f || freq_ == 0.0f)
          return;

        auto frames = viewFrames<Format>(buffer);
        float frame_tm = 1.0 / buffer.spec().rate;
        float phase_os = phase_ / (2.0 * M_PI);
        auto& tbl_page = tbl_.lookup(freq_);

        std::array<float,2> start = {
          phase_os,
          phase_os
        };
        std::array<float,2> step = {
          (freq_ - detune_) * frame_tm,
          (freq_ + detune_) * frame_tm
        };
        tbl_page.lookup(frames.begin(), frames.end(), std::move(start), std::move(step),
                        [&] (auto& frame, auto& values)
                          {
                            frame += {
                              amp_ * values[0],
                              amp_ * values[1]
                            };
                          });
      }
  private:
    const Wavetable& tbl_;
    float freq_;
    float amp_;
    float phase_;
    float detune_;
  };

  /**
   * @class Normalize
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Normalize {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    Normalize(const Decibel& level_l, const Decibel& level_r)
      : amp_l_{static_cast<float>(level_l.amplitude())},
        amp_r_{static_cast<float>(level_r.amplitude())}
      {/*nothing*/}

    Normalize(float amp_l, float amp_r) : amp_l_{amp_l}, amp_r_{amp_r}
      {/*nothing*/}

    explicit Normalize(const Decibel& level = 0_dB) : Normalize(level, level)
      {/*nothing*/}

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }

    void process(ByteBuffer& buffer)
      {
        assert(buffer.spec().channels == 2);
        auto frames = viewFrames<Format>(buffer);
        float max = 0.0f;
        for (auto& frame : frames)
          max = std::max(max, std::max(std::abs(frame[0]), std::abs(frame[1])));

        if (max != 0.0f)
          for (auto& frame : frames)
            frame *= { amp_l_ / max, amp_r_ / max};
      }
  private:
    float amp_l_;
    float amp_r_;
  };

  /**
   * @class Mix
   *
   */
  template<SampleFormat Format = kDefaultSampleFormat>
  class Mix {

    static_assert(isFloatingPoint(Format),
      "this filter only supports floating point types");

  public:
    Mix(const ByteBuffer* buffer = nullptr) : buffer_{buffer}
      {/*nothing*/}

    void setBuffer(const ByteBuffer* buffer)
      { buffer_ = buffer; }

    void prepare(const StreamSpec& spec)
      {
        assert( isFloatingPoint(spec.format) );
        assert( spec.channels == 2 );
      }

    void process(ByteBuffer& buffer)
      {
        if (buffer_ == nullptr)
          return;

        auto frames1 = viewFrames<Format>(buffer);
        auto frames2 = viewFrames<Format>(*buffer_);

        auto it2 = frames2.begin();
        for (auto it1 = frames1.begin();
             it1 != frames1.end() && it2 != frames2.end();
             ++it1, ++it2)
        {
          *it1 += *it2;
        }
      }
  private:
    const ByteBuffer* buffer_;
  };

  namespace std {

    // some shortcuts for default filters
    using FIR       = Filter<FIR<kDefaultSampleFormat>>;
    using Lowpass   = Filter<Lowpass<kDefaultSampleFormat>>;
    using Zero      = Filter<Zero<kDefaultSampleFormat>>;
    using Gain      = Filter<Gain<kDefaultSampleFormat>>;
    using Noise     = Filter<Noise<kDefaultSampleFormat>>;
    using Wave      = Filter<Wave<kDefaultSampleFormat>>;
    using Normalize = Filter<Normalize<kDefaultSampleFormat>>;
    using Mix       = Filter<Mix<kDefaultSampleFormat>>;

  }//namespace std

} //namespace filter
}//namespace audio
#endif//GMetronome_Filter_h
