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

#ifndef GMetronome_View_h
#define GMetronome_View_h

#include <iterator>
#include <algorithm>

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

/*
template<typename ContainerViewType>
class ElementSelectView : public ContainerViewType::ProxyViewType
{
public:

  using ProxyViewType = typename ContainerViewType::ProxyViewType;
  using StorageIterator = typename ProxyViewType::StorageIterator;

  ElementSelectView(ContainerViewType& container, std::size_t select) : select_{select}
    {}

  void align(StorageIterator iter)
    {
      ProxyViewType::align(iter + select_ * ProxyViewType::extent());
    }

private:
  std::size_t select_;
};
*/
#endif//GMetronome_View_h
