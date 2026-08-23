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

#ifndef GMetronome_Result_h
#define GMetronome_Result_h

#include <type_traits>
#include <utility>

/**
 * @class Result
 * @brief Represents either an expected value of type T, or an error value of type E.
 *
 * Can be replaced by std::expected when switching to C++23.
 */
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

/** Partial specialization for void.*/
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

#endif//GMetronome_Result_h
