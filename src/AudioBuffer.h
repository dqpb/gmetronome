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

#ifndef GMetronome_AudioBuffer_h
#define GMetronome_AudioBuffer_h

#include "Audio.h"

#include <vector>
#include <chrono>
#include <initializer_list>
#include <algorithm>
#include <iterator>
#include <array>
#include <cmath>
#include <cstring>
#include <cassert>

namespace audio {

  using Byte = uint8_t;

  /**
   * @class ByteBuffer
   *
   * A ByteBuffer aggregates interleaved audio sample data and a stream
   * specification (sample format, rate and number of channels).
   * It provides byte level access and forms the data base for the higher
   * level abstractions (views) like samples, frames etc.
   */
  class ByteBuffer {
  public:
    using  container_type  = typename std::vector<Byte>;
    using  value_type      = typename container_type::value_type;
    using  allocator_type  = typename container_type::allocator_type;
    using  size_type       = typename container_type::size_type;
    using  difference_type = typename container_type::difference_type;
    using  reference       = typename container_type::reference;
    using  const_reference = typename container_type::const_reference;
    using  pointer         = typename container_type::pointer;
    using  const_pointer   = typename container_type::const_pointer;
    using  iterator        = typename container_type::iterator;
    using  const_iterator  = typename container_type::const_iterator;

  public:
    ByteBuffer(const StreamSpec& spec = kDefaultSpec, size_type count = 0)
      : spec_{spec},
        data_(count, 0)
      {}

    ByteBuffer(const StreamSpec& spec, microseconds duration)
      : spec_{spec},
        data_(usecsToBytes(duration,spec), 0)
      {}

    ByteBuffer(const StreamSpec& spec, container_type data)
      : spec_{spec},
        data_{std::move(data)}
      {}

    pointer data()
      { return data_.data(); }
    const_pointer data() const
      { return data_.data(); }
    const StreamSpec& spec() const
      { return spec_; }

    void resample(const StreamSpec& spec);

    microseconds time() const
      { return bytesToUsecs(data_.size(), spec_); }

    bool operator==(const ByteBuffer& other) const
      { return data_ == other.data_ && spec_ == other.spec_; }
    bool operator!=(const ByteBuffer& other) const
      { return !((*this) == other); }

    iterator begin()
      { return data_.begin(); }
    const_iterator begin() const
      { return data_.begin(); }
    const_iterator cbegin() const
      { return data_.cbegin(); }
    iterator end()
      { return data_.end(); }
    const_iterator end() const
      { return data_.end(); }
    const_iterator cend() const
      { return data_.cend(); }

    reference operator[]( size_type pos )
      { return data_[pos]; }
    const_reference operator[]( size_type pos ) const
      { return data_[pos]; }

    void swap(ByteBuffer& other)
      {
        std::swap(data_, other.data_);
        std::swap(spec_, other.spec_);
      }

    size_type size() const
      { return data_.size(); }
    size_type max_size() const
      { return data_.max_size(); }
    size_type frames() const
      {
        auto fs = frameSize(spec_);
        return (fs == 0) ? 0 : data_.size() / fs;
      }
    SampleFormat format() const
      { return spec_.format; }
    SampleRate rate() const
      { return spec_.rate; }
    size_type channels() const
      { return spec_.channels; }
    size_type samples() const
      { return data_.size() / sampleSize(spec_.format); }
    bool empty() const
      { return data_.empty(); }

  private:
    StreamSpec spec_;
    container_type data_;
  };

  /**
   * @class View
   * @brief Base class for proxy objects to access data in a storage
   */
  template<typename StoreIter>
  struct View
  {
    using StorageIterator = StoreIter;

    explicit View(StoreIter iter) : iter_{iter} {}

    StoreIter alignment() const
      { return iter_; }
    void align(StoreIter iter)
      { iter_ = iter; }
    std::size_t extent() const;

  private:
    StoreIter iter_;
  };

  /**
   * @class StrideIterator
   */
  template<typename ProxyView>
  class StrideIterator {
  public:
    // traits
    using ProxyViewType     = ProxyView;
    using difference_type   = std::size_t;
    using value_type        = ProxyViewType;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::forward_iterator_tag;

    explicit StrideIterator(ProxyViewType view) : view_(std::move(view))
      {}
    StrideIterator& operator++()
      {view_.align(view_.alignment() + view_.extent()); return *this;}
    StrideIterator operator++(int)
      {StrideIterator retval = *this; ++(*this); return retval;}
    bool operator==(StrideIterator other) const
      {return view_.alignment() == other.view_.alignment();}
    bool operator!=(StrideIterator other) const
      {return !(*this == other);}
    ProxyView& operator*()
      { return view_; }
    ProxyView* operator->()
      { return &view_; }

  private:
    ProxyView view_;
  };

  /**
   * @class ContainerView<V,...>
   */
  template<typename ProxyView, typename StoreIter>
  struct ContainerView : public View<StoreIter>
  {
    using ProxyViewType        = ProxyView;
    using Iterator             = StrideIterator<ProxyView>;

    static_assert(std::is_base_of_v<View<StoreIter>, ProxyView>);

    template<typename... ProxyArgs>
    ContainerView(StoreIter iter, size_t size, ProxyArgs... args)
      : View<StoreIter>(iter), size_{size}, proxy_view_(iter, args...)
      {};

    StoreIter alignment() const
      { return View<StoreIter>::alignment(); }

    std::size_t extent() const
      { return size_ * proxy_view_.extent(); }

    std::size_t size() const
      { return size_; }

    template<typename OtherProxyView, typename OtherStoreIter>
    ContainerView& operator=(const ContainerView<OtherProxyView, OtherStoreIter>& other)
      {
        std::copy_n(other.begin(), std::min(size(), other.size()), begin());
        return *this;
      }

    ProxyView& operator[](std::size_t index)
      {
        proxy_view_.align(alignment() + index * proxy_view_.extent());
        return proxy_view_;
      }
    Iterator begin()
      {
        ProxyViewType tmp = proxy_view_;
        tmp.align(alignment());
        return Iterator(tmp);
      }
    Iterator end()
      {
        ProxyViewType tmp = proxy_view_;
        tmp.align(alignment() + extent());
        return Iterator(tmp);
      }
    Iterator begin() const
      {
        ProxyView tmp = proxy_view_;
        tmp.align(alignment());
        return Iterator(tmp);
      }
    Iterator end() const
      {
        ProxyView tmp = proxy_view_;
        tmp.align(alignment() + extent());
        return Iterator(tmp);
      }

  private:
    std::size_t size_;
    ProxyView proxy_view_;
  };

  /**
   * @class ContainerView<V,N,...>
   * @brief Partial specialization for compile time sized container views
   */
/*
  template<typename V, typename BytePointerType std::size_t N, std::size_t...Rest>
  class ContainerView<V, N, Rest...> : public View {
  public:
  using ViewType = V;
  using Iterator = StrideIterator<ViewType>;
  static constexpr std::size_t kSize = N;

  template<typename...VArgs>
  ContainerView(BytePointer ptr, VArgs...vargs)
  : View(ptr), view_proxy_{ptr, vargs...}
  {}

  template<typename OtherContainer>
  ContainerView<V,Rest...>& operator=(const OtherContainer& other)
  {
  std::copy_n(other.begin(), std::min(size(), other.size()), begin());
  return *this;
  }

  std::size_t extent() const
  { return size() * view_proxy_.extent(); }

  static constexpr std::size_t size()
  { return kSize; }

  ViewType& operator[](std::size_t index)
  {
  view_proxy_.align(alignment() + index * view_proxy_.extent());
  return view_proxy_;
  }
  Iterator begin()
  {
  ViewType tmp = view_proxy_;
  tmp.align(alignment());
  return Iterator(tmp);
  }
  Iterator end()
  {
  ViewType tmp = view_proxy_;
  tmp.align(alignment() + extent());
  return Iterator(tmp);
  }
  private:
  ViewType view_proxy_;
  };
*/


  /**
   * @class SampleView
   * @brief View to access an audio sample in an underlying byte storage
   */
  template<SampleFormat Format, typename StoreIter>
  class SampleView : public View<StoreIter> {
  public:
    using ValueType = typename SampleValueType<Format>::type;

    explicit SampleView(StoreIter iter) : View<StoreIter>(iter)
      {}
    explicit SampleView(ByteBuffer& buffer) : View<StoreIter>(buffer.data())
      { assert(buffer.size() >= sampleSize(Format)); }
    explicit SampleView(const ByteBuffer& buffer) : View<StoreIter>(buffer.data())
      { assert(buffer.size() >= sampleSize(Format)); }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    SampleView& operator=(const SampleView<OtherFormat, OtherStoreIter>& other)
      {
        if constexpr (OtherFormat == Format)
          copy(other);
        else
          convert<OtherFormat,OtherStoreIter> (
            other, FormatTrait_t<OtherFormat>{}, FormatTrait_t<Format>{});

        return *this;
      }

    static constexpr bool hasSwapEndian()
      {
        return sampleEndian(Format) != Endian::kUnknown
          && hostEndian() != Endian::kUnknown
          && sampleEndian(Format) != hostEndian();
      }

    StoreIter alignment() const
      { return View<StoreIter>::alignment(); }

    static constexpr std::size_t extent()
      { return sampleSize(Format); }

    auto& operator=(ValueType value)
      {
        if constexpr (hasSwapEndian())
          return storeSwap(value);
        else
          return store(value);
      }

    operator ValueType() const
      {
        if constexpr (hasSwapEndian())
          return loadSwap();
        else
          return load();
      }

    template<typename T> SampleView& operator+=(const T& value)
      {
        (*this) = (*this) + value;
        return *this;
      }
    template<typename T> SampleView& operator-=(const T& value)
      {
        (*this) = (*this) - value;
        return *this;
      }
    template<typename T> SampleView& operator*=(const T& value)
      {
        (*this) = (*this) * value;
        return *this;
      }
    template<typename T> SampleView& operator/=(const T& value)
      {
        (*this) = (*this) / value;
        return *this;
      }

  private:
    // tags
    struct signed_integral {};
    struct unsigned_integral {};
    struct floating_point {};
    struct integral {};

    template<SampleFormat F>
    struct FormatTrait
    {
      using type = typename std::conditional_t<
        isIntegral(F), integral, floating_point>;
    };

    template<SampleFormat F>
    using FormatTrait_t = typename FormatTrait<F>::type;

    ValueType swapEndian(ValueType value) const
      {
        static_assert(sizeof(ValueType) == extent());
        static_assert(std::is_trivial_v<ValueType> && std::is_standard_layout_v<ValueType>);

        if constexpr (extent() == 1)
        {}
        else if constexpr (extent() == 2 && std::is_integral_v<ValueType>)
        {
          value = ((value & 0x00ff) <<  8) | ((value >>  8) & 0x00ff);
        }
        else if constexpr (extent() == 4 && std::is_integral_v<ValueType>)
        {
          value = ((value & 0x0000ffff) << 16) | ((value >> 16) & 0x0000ffff);
          value = ((value & 0x00ff00ff) <<  8) | ((value >>  8) & 0x00ff00ff);
        }
        else
        {
          std::array<Byte, extent()> bytes;
          std::memcpy(bytes.data(), &value, extent());
          std::reverse(bytes.begin(), bytes.end());
          std::memcpy(&value, bytes.data(), extent());
        }
        return value;
      }

    auto& store(ValueType value)
      {
        static_assert(sizeof(ValueType) == extent());
        std::memcpy(alignment(), &value, extent());
        return *this;
      }

    auto& storeSwap(ValueType value)
      { return store(swapEndian(value)); }

    ValueType load() const
      {
        static_assert(sizeof(ValueType) == extent());
        ValueType value;
        std::memcpy(&value, alignment(), extent());
        return value;
      }

    ValueType loadSwap() const
      { return swapEndian(load()); }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    void copy(const SampleView<OtherFormat, OtherStoreIter>& other)
      {
        std::copy_n(other.alignment(), extent(), alignment());
      }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    void convert(const SampleView<OtherFormat, OtherStoreIter>& other,
                 /*from*/ integral, /*to*/ integral)
      {
        using OtherType = typename SampleValueType<OtherFormat>::type;
        using OtherTypeSigned = std::make_signed_t<OtherType>;

        constexpr long long offset = []() -> long long {
          if constexpr (isUnsigned(OtherFormat) && isSigned(Format))
            return std::numeric_limits<OtherTypeSigned>::min();
          else if constexpr (isSigned(OtherFormat) && isUnsigned(Format))
            return -1ll * std::numeric_limits<OtherType>::min();
          else
            return 0;
        }();

        constexpr int bitshift = (sampleSize(Format) - sampleSize(OtherFormat)) * 8;

        if constexpr (bitshift >= 0)
          (*this) = (other + offset) << bitshift;
        else
          (*this) = (other + offset) >> -bitshift;
      }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    void convert(const SampleView<OtherFormat, OtherStoreIter>& other,
                 /*from*/ floating_point, /*to*/ integral)
      {
        using ValueTypeSigned = std::make_signed_t<ValueType>;

        constexpr long offset = []() -> long {
          if constexpr (isUnsigned(Format))
            return std::numeric_limits<ValueTypeSigned>::min();
          else
            return 0;
        }();

        (*this) = (std::clamp(double(other), -1.0, 1.0)
                   * std::numeric_limits<ValueTypeSigned>::max()) - offset;
      }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    void convert(const SampleView<OtherFormat, OtherStoreIter>& other,
                 /*from*/ integral, /*to*/ floating_point)
      {
        using OtherType = typename SampleValueType<OtherFormat>::type;
        using OtherTypeSigned = std::make_signed_t<OtherType>;

        constexpr long offset = []() -> long {
          if constexpr (isUnsigned(OtherFormat))
            return std::numeric_limits<OtherTypeSigned>::min();
          else
            return 0;
        }();

        (*this) = (other + offset) / (double) -std::numeric_limits<OtherTypeSigned>::min();
      }

    template<SampleFormat OtherFormat, typename OtherStoreIter>
    void convert(const SampleView<OtherFormat, OtherStoreIter>& other,
                 /*from*/ floating_point, /*to*/ floating_point)
      {
        (*this) = other;
      }
  };

  /**
   * @class SampleContainerView
   */
  template<SampleFormat Format, typename StoreIter>
  struct SampleContainerView : public ContainerView<SampleView<Format, StoreIter>, StoreIter>
  {
    using ProxyViewType = SampleView<Format, StoreIter>;
    using Base = ContainerView<ProxyViewType, StoreIter>;

    using Base::operator=;

    SampleContainerView(StoreIter iter, size_t size)
      : ContainerView<ProxyViewType, StoreIter>(iter, size)
      {}
    explicit SampleContainerView(ByteBuffer& buffer)
      : ContainerView<ProxyViewType, StoreIter>(buffer.data(), buffer.samples())
      {}
    explicit SampleContainerView(const ByteBuffer& buffer)
      : ContainerView<ProxyViewType, StoreIter>(buffer.data(), buffer.samples())
      {}
  };

  /**
   * @class FrameView
   */
  template<SampleFormat Format, typename StoreIter>
  struct FrameView : public ContainerView<SampleView<Format, StoreIter>, StoreIter>
  {
    using ProxyViewType = SampleView<Format, StoreIter>;
    using Base = ContainerView<ProxyViewType, StoreIter>;

    // ctors
    FrameView(StoreIter iter, size_t size)
      : ContainerView<ProxyViewType, StoreIter>(iter, size)
      {}
    explicit FrameView(ByteBuffer& buffer)
      : ContainerView<ProxyViewType, StoreIter>(buffer.data(), buffer.channels())
      {}
    explicit FrameView(const ByteBuffer& buffer)
      : ContainerView<ProxyViewType, StoreIter>(buffer.data(), buffer.channels())
      {}

    // copy assign
    template<SampleFormat OtherFormat, typename OtherStoreIter>
    FrameView& operator=(const FrameView<OtherFormat, OtherStoreIter>& other)
      {
        Base::operator=(other);
        return *this;
      }
    template<typename T>
    FrameView& operator=(const std::initializer_list<T>& list)
      {
        std::copy_n(list.begin(), std::min(Base::size(), list.size()), Base::begin());
        return *this;
      }
    template<typename T>
    FrameView& operator=(const T& value)
      {
        std::fill(Base::begin(), Base::end(), value);
        return *this;
      }

    // assign ops
    template<SampleFormat OtherFormat, typename OtherStoreIter>
    FrameView& operator+=(const FrameView<OtherFormat, OtherStoreIter>& other)
      {
        applyElementwise(Base::begin(), Base::end(), other.begin(), other.end(),
                         [] (auto& lhs, const auto& rhs) { lhs += rhs; });
        return *this;
      }
    template<SampleFormat OtherFormat, typename OtherStoreIter>
    FrameView& operator-=(const FrameView<OtherFormat, OtherStoreIter>& other)
      {
        applyElementwise(Base::begin(), Base::end(), other.begin(), other.end(),
                         [] (auto& lhs, const auto& rhs) { lhs -= rhs; });
        return *this;
      }
    template<SampleFormat OtherFormat, typename OtherStoreIter>
    FrameView& operator*=(const FrameView<OtherFormat, OtherStoreIter>& other)
      {
        applyElementwise(Base::begin(), Base::end(), other.begin(), other.end(),
                         [] (auto& lhs, const auto& rhs) { lhs *= rhs; });
        return *this;
      }
    template<SampleFormat OtherFormat, typename OtherStoreIter>
    FrameView& operator/=(const FrameView<OtherFormat, OtherStoreIter>& other)
      {
        applyElementwise(Base::begin(), Base::end(), other.begin(), other.end(),
                         [] (auto& lhs, const auto& rhs) { lhs /= rhs; });
        return *this;
      }

    template<typename T>
    FrameView& operator+=(const std::initializer_list<T>& list)
      {
        applyElementwise(Base::begin(), Base::end(), list.begin(), list.end(),
                         [] (auto& lhs, const auto& rhs) { lhs += rhs; });
        return *this;
      }
    template<typename T>
    FrameView& operator-=(const std::initializer_list<T>& list)
      {
        applyElementwise(Base::begin(), Base::end(), list.begin(), list.end(),
                         [] (auto& lhs, const auto& rhs) { lhs -= rhs; });
        return *this;
      }
    template<typename T>
    FrameView& operator*=(const std::initializer_list<T>& list)
      {
        applyElementwise(Base::begin(), Base::end(), list.begin(), list.end(),
                         [] (auto& lhs, const auto& rhs) { lhs *= rhs; });
        return *this;
      }
    template<typename T>
    FrameView& operator/=(const std::initializer_list<T>& list)
      {
        applyElementwise(Base::begin(), Base::end(), list.begin(), list.end(),
                         [] (auto& lhs, const auto& rhs) { lhs /= rhs; });
        return *this;
      }

    template<typename T> FrameView& operator+=(const T& value)
      {
        for (auto& sample : *this) sample += value;
        return *this;
      }
    template<typename T> FrameView& operator-=(const T& value)
      {
        for (auto& sample : *this) sample -= value;
        return *this;
      }
    template<typename T> FrameView& operator*=(const T& value)
      {
        for (auto& sample : *this) sample *= value;
        return *this;
      }
    template<typename T> FrameView& operator/=(const T& value)
      {
        for (auto& sample : *this) sample /= value;
        return *this;
      }

  private:
    template<typename InputIt1, typename InputIt2, typename BinaryOperation>
    void applyElementwise(InputIt1 first1, InputIt1 last1,
                          InputIt2 first2, InputIt2 last2, BinaryOperation op)
      {
        auto it2 = first2;
        for (auto it1 = first1; it1 != last1 && it2 != last2; ++it1, ++it2)
          op(*it1, *it2);
      }
  };

  /**
   * @class FrameContainerView
   */
  template<SampleFormat Format, typename StoreIter>
  struct FrameContainerView : public ContainerView<FrameView<Format, StoreIter>, StoreIter>
  {
    using ProxyViewType = FrameView<Format, StoreIter>;
    using Base = ContainerView<ProxyViewType, StoreIter>;

    using Base::operator=;

    FrameContainerView(StoreIter iter, size_t frames, size_t channels)
      : Base(iter, frames, channels)
      {}
    explicit FrameContainerView(ByteBuffer& buffer)
      : Base(buffer.data(), buffer.frames(), buffer.channels())
      {}
    explicit FrameContainerView(const ByteBuffer& buffer)
      : Base(buffer.data(), buffer.frames(), buffer.channels())
      {}
  };

  /**
   * @class ChannelSampleView
   */
  template<SampleFormat Format, typename StoreIter>
  struct ChannelSampleView : public SampleView<Format, StoreIter>
  {
    using Base = SampleView<Format, StoreIter>;

    using Base::operator=;

    // ctors
    ChannelSampleView(StoreIter iter, size_t channels)
      : Base(iter), channels_{channels}
      {}

    std::size_t extent() const
      { return Base::extent() * channels_; }

  private:
    size_t channels_;
  };

  /**
   * @class ChannelView
   */
  template<SampleFormat Format, typename StoreIter>
  struct ChannelView : public ContainerView<ChannelSampleView<Format, StoreIter>, StoreIter>
  {
    using ProxyViewType = ChannelSampleView<Format, StoreIter>;
    using Base = ContainerView<ProxyViewType, StoreIter>;

    using Base::operator=;

    ChannelView(StoreIter iter, size_t frames, size_t channels)
      : Base(iter, frames, channels)
      {}
    explicit ChannelView(ByteBuffer& buffer, size_t select)
      : Base(buffer.data() + sampleSize(Format) * select, buffer.frames(), buffer.channels())
      {}
    explicit ChannelView(const ByteBuffer& buffer, size_t select)
      : Base(buffer.data() + sampleSize(Format) * select, buffer.frames(), buffer.channels())
      {}
    constexpr std::size_t extent() const
      { return SampleView<Format, StoreIter>::extent(); }
  };

  /**
   * @class ChannelContainerView
   */
  template<SampleFormat Format, typename StoreIter>
  struct ChannelContainerView : public ContainerView<ChannelView<Format, StoreIter>, StoreIter>
  {
    using ProxyViewType = ChannelView<Format, StoreIter>;
    using Base = ContainerView<ProxyViewType, StoreIter>;

    using Base::operator=;

    ChannelContainerView(StoreIter iter, size_t channels, size_t frames)
      : Base(iter, channels, frames, channels)
      {}
    explicit ChannelContainerView(ByteBuffer& buffer)
      : Base(buffer.data(), buffer.channels(), buffer.frames(), buffer.channels())
      {}
    explicit ChannelContainerView(const ByteBuffer& buffer)
      : Base(buffer.data(), buffer.channels(), buffer.frames(), buffer.channels())
      {}
  };

  // helper to construct views from byte buffers
  template<SampleFormat Format>
  auto viewSample(ByteBuffer& buffer)
  { return SampleView<Format, ByteBuffer::pointer>(buffer); }

  template<SampleFormat Format>
  auto viewSample(const ByteBuffer& buffer)
  { return SampleView<Format, ByteBuffer::const_pointer>(buffer); }

  template<SampleFormat Format>
  auto viewSamples(ByteBuffer& buffer)
  { return SampleContainerView<Format, ByteBuffer::pointer>(buffer); }

  template<SampleFormat Format>
  auto viewSamples(const ByteBuffer& buffer)
  { return SampleContainerView<Format, ByteBuffer::const_pointer>(buffer); }

  template<SampleFormat Format>
  auto viewFrame(ByteBuffer& buffer)
  { return FrameView<Format, ByteBuffer::pointer>(buffer); }

  template<SampleFormat Format>
  auto viewFrame(const ByteBuffer& buffer)
  { return FrameView<Format, ByteBuffer::const_pointer>(buffer); }

  template<SampleFormat Format>
  auto viewFrames(ByteBuffer& buffer)
  { return FrameContainerView<Format, ByteBuffer::pointer>(buffer); }

  template<SampleFormat Format>
  auto viewFrames(const ByteBuffer& buffer)
  { return FrameContainerView<Format, ByteBuffer::const_pointer>(buffer); }

  template<SampleFormat Format>
  auto viewChannel(ByteBuffer& buffer, size_t channel)
  { return ChannelView<Format, ByteBuffer::pointer>(buffer, channel); }

  template<SampleFormat Format>
  auto viewChannel(const ByteBuffer& buffer, size_t channel)
  { return ChannelView<Format, ByteBuffer::const_pointer>(buffer, channel); }

  template<SampleFormat Format>
  auto viewChannels(ByteBuffer& buffer)
  { return ChannelContainerView<Format, ByteBuffer::pointer>(buffer); }

  template<SampleFormat Format>
  auto viewChannels(const ByteBuffer& buffer)
  { return ChannelContainerView<Format, ByteBuffer::const_pointer>(buffer); }

  // resample an audio buffer
  ByteBuffer resample(const ByteBuffer& buffer, const StreamSpec& to_spec);

}//namespace audio
#endif//GMetronome_AudioBuffer_h
