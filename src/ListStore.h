/*
 * Copyright (C) 2026 The GMetronome Team
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

#ifndef GMetronome_ListStore_h
#define GMetronome_ListStore_h

#include <vector>
#include <string>
#include <sigc++/sigc++.h>

template<typename T, typename E>
class Result {
public:
  using Type = T;
  using Error = E;

public:
  Result() : has_value_{true}
    { /* nothing */ }

  template<class U,
           std::enable_if_t<
             std::is_constructible_v<Type, U&&> &&
             !std::is_constructible_v<Error, U&&>, int> = 0>
  Result(U&& value) : has_value_{true}, value_{std::forward<U>(value)}
    { /* nothing */ }

  template<class G,
           std::enable_if_t<
             std::is_constructible_v<Error, G&&> &&
             !std::is_constructible_v<Type, G&&>, int> = 0>
  Result(G&& error) : error_{std::forward<G>(error)}
    { /* nothing */ }

  explicit operator bool() const
    { return has_value_; }

  bool hasValue() const
    { return has_value_; }

  const Type& value() const &
    { return value_; }
  Type& value() &
    { return value_; }
  Type&& value() &&
    { return std::move(value_); }

  const Type& operator*() const &
    { return value_; }
  Type& operator*() &
    { return value_; }

  const Type* operator->() const
    { return &value_; }
  Type* operator->()
    { return &value_; }

  const Error& error() const &
    { return error_; }
  Error& error() &
    { return error_; }
  Error&& error() &&
    { return std::move(error_); }

private:
  bool has_value_{false};
  Type value_;
  Error error_;
};

/** Partial specialization */
template<typename E>
class Result<void,E> {
public:
  using Type = void;
  using Error = E;

public:
  Result() : has_value_{true}
    { /* nothing */ }

  template<class G, std::enable_if_t<std::is_constructible_v<Error, G&&>, int> = 0>
  Result(G&& error) : error_{std::forward<G>(error)}
    { /* nothing */ }

  explicit operator bool() const
    { return has_value_; }

  bool hasValue() const
    { return has_value_; }

  const Error& error() const &
    { return error_; }
  Error& error() &
    { return error_; }
  Error&& error() &&
    { return std::move(error_); }

private:
  bool has_value_{false};
  Error error_;
};

/**
 * @brief  Generic interface for persistent lists.
 *
 * @tparam T  The item type to be stored
 * @tparam I  An identifier type
 * @tparam H  A header type usually containing a title or short description
 *            of the item that can be used in a UI list.
 */
template<typename T, typename I, typename H>
class ListStore {
public:
  using Type = T;
  using Header = H;
  using Identifier = I;

  struct Primer
  {
    Identifier id;
    Header header;
  };

  struct Patch {
    virtual ~Patch() = default;
    virtual void apply(T& item) const = 0;
  };

  struct Error {
    enum class Category
    {
      kNone,
      kNotFound,
      kIO,
      kParse,
      kSerialization,
      kValidation,
      kConflict,
      kUnavailable,
      kUnknown
    };
    Category category {Category::kNone};
    std::string what;
    std::string detail;
  };

  template<typename R>
  using Result = Result<R,Error>;

public:
  virtual ~ListStore() {}

  /**
   * Returns an up-to-date list of primers (@link struct Primer) of all
   * stored items.  These primers contain i.a. the identifier which can
   * later be used to fully load a specific item via @link load @endlink.
   *
   * @return A vector of item primers.
   */
  virtual Result<std::vector<Primer>> list() = 0;

  /**
   * Load the item with the identifier id from the underlying data storage.
   * A list of valid identifiers can be obtained by using list() method.
   *
   * @param id Identifier of the item.
   * @return The loaded item.
   */
  virtual Result<Type> load(const Identifier& id) = 0;

  /**
   * Store an item in the underlying data storage.
   *
   * @param id The item identifier.
   * @param item The item to store.
   */
  virtual Result<void> store(const Identifier& id, const Type& item) = 0;

  /**
   * Remove an item from the underlying data storage.
   *
   * @param id The identifier of the item to delete.
   */
  virtual Result<void> remove(const Identifier& id) = 0;

  /**
   * Update an item in the underlying data storage.
   *
   * @param id The item identifier.
   * @param patch The patch to apply to the item.
   */
  virtual Result<void> update(const Identifier& id, const Patch& patch) = 0;

  /**
   * Change the order of the stored items.
   *
   * @param  A vector of item identifiers.
   */
  virtual Result<void> reorder(const std::vector<Identifier>& order) = 0;

  /**
   * Realize all pending changes.
   *
   * A concrete implementation of this interface might cache item changes
   * and update the underlying data storage later. This method forces the
   * synchronization between the internal module data and the data storage.
   */
  virtual Result<void> flush() { return {}; }

  /**
   * Implementations of this interface should emit this signal if a modification
   * of items in the underlying data storage (e.g. a file modification) has been
   * detected so that the client can take actions to synchronize with the UI data.
   */
  sigc::signal<void> signal_storage_changed()
  { return signal_storage_changed_; }

protected:
  sigc::signal<void> signal_storage_changed_;
};

#endif//GMetronome_ListStore_h
