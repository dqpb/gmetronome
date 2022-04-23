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

#ifndef GMetronome_ObjectLibrary_h
#define GMetronome_ObjectLibrary_h

#include <algorithm>
#include <map>
#include <tuple>
#include <utility>

/**
 * @class ObjectLibrary
 * @brief Maintains a collection of objects that need common resources for
 *        construction and modification
 *
 * Object constructions and updates are not applied immediately but 'lazily'
 * delayed until get() or apply() is called (i.e. the object is needed or
 * explicitly updated).
 *
 * The builder (BuilderType) needs to implement the following functions:
 *   1) void prepare(PreparationArguments...)
 *   2) ObjectType create(ObjectParameters...)
 *   3) void update(ObjectType& obj, ObjectParameters...)
 */
template<typename KeyType, typename ObjectType, typename BuilderType>
class ObjectLibrary {
public:
  /**
   * @brief Constructs an object library.
   * The arguments will be forwarded to the builder's constructor.
   */
  template<typename ...Args> ObjectLibrary(Args&&... args);

  /**
   * @brief Prepare the builder
   * The arguments will be forwarded to the builder's prepare function.
   */
  template<typename ...Args> void prepare(Args&&... args);

  /**
   * @brief Inserts a new object with the given key
   * If an object with the key already exists, the object will be updated.
   */
  template<typename ...Args>
  void insert(const KeyType& key, Args&&... args);

  /**
   * @brief Returns the object with the given key
   * @note Pending updates for the requested object will be applied before.
   */
  const ObjectType& get(KeyType key);
  const ObjectType& operator[](const KeyType& key);

  /** @brief Removes the object with the given key */
  void erase(const KeyType& key);

  /**
   * @brief Updates a previously inserted object
   * @note This function just stores the given parameters. The actual update
   *       will occur when get() or apply() is called.
   */
  template<typename ...Args>
  void update(const KeyType& key, Args&&... args);

  /** Applies pending updates for the object with the given key. */
  void apply(const KeyType& key);

  /** Applies pending updates for all registered objects. */
  void apply();

  /** Remove all objects. */
  void clear();

  /** Checks if the library contains an object with the given key. */
  bool contains(const KeyType& key) const;

  /** Checks if the object with the given key has pending updates. */
  bool pending(const KeyType& key) const;

  /** Checks if the library contains objects with pending updates. */
  bool pending() const;

private:
  BuilderType builder_;

  template<typename Fu> struct fu_arguments;

  template<typename ...Args>
  struct fu_arguments<ObjectType (BuilderType::*)(Args...)>
  { using type = std::tuple<typename std::decay<Args>::type...>; };

  using ObjectParameters = typename fu_arguments<decltype(&BuilderType::create)>::type;

  struct MetaMapEntry_
  {
    ObjectParameters params;
    bool pending {true};
  };
  std::map<KeyType, ObjectType> object_map_;
  std::map<KeyType, MetaMapEntry_> meta_map_;
};

template<typename KeyType, typename ObjectType, typename BuilderType>
template<typename ...Args>
ObjectLibrary<KeyType, ObjectType, BuilderType>::ObjectLibrary(Args&&... args)
  : builder_{std::forward<Args>(args)...}
{ /* nothing */ }

template<typename KeyType, typename ObjectType, typename BuilderType>
template<typename ...Args>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::prepare(Args&&... args)
{
  builder_.prepare(std::forward<Args>(args)...);
  std::for_each(meta_map_.begin(), meta_map_.end(),
                [] (auto& map_pair) { map_pair.second.pending = true; });
}

template<typename KeyType, typename ObjectType, typename BuilderType>
template<typename ...Args>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::insert(const KeyType& key, Args&&... args)
{
  meta_map_.insert_or_assign(key, MetaMapEntry_{std::move(args)..., true});
}

template<typename KeyType, typename ObjectType, typename BuilderType>
const ObjectType&
ObjectLibrary<KeyType, ObjectType, BuilderType>::get(KeyType key)
{
  apply(key);
  return object_map_.at(key);
}

template<typename KeyType, typename ObjectType, typename BuilderType>
const ObjectType&
ObjectLibrary<KeyType, ObjectType, BuilderType>::operator[](const KeyType& key)
{
  return get(key);
}

template<typename KeyType, typename ObjectType, typename BuilderType>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::erase(const KeyType& key)
{
  object_map_.erase(key);
  meta_map_.erase(key);
}

template<typename KeyType, typename ObjectType, typename BuilderType>
template<typename ...Args>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::update(const KeyType& key, Args&&... args)
{
  auto& entry = meta_map_.at(key);
  entry.params = {std::move(args)...};
  entry.pending = true;
}

template<typename KeyType, typename ObjectType, typename BuilderType>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::apply(const KeyType& key)
{
  if (auto& meta = meta_map_.at(key); meta.pending)
  {
    if (auto obj_it = object_map_.find(key); obj_it != object_map_.end())
    {
      // update existing object
      std::apply( [&] (auto&&... args) {
        builder_.update(obj_it->second, std::forward<decltype(args)>(args)...);
      }, meta.params);
    }
    else // create new object
    {
      object_map_.insert(
        std::make_pair(key, std::apply( [&] (auto&&... args) {
          return builder_.create(std::forward<decltype(args)>(args)...);
        }, meta.params)));
    }
    meta.pending = false;
  }
}

template<typename KeyType, typename ObjectType, typename BuilderType>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::apply()
{
  // TODO: parallelize
  std::for_each(meta_map_.begin(), meta_map_.end(),
                [this] (auto& map_pair) { apply(map_pair.first); });
}

template<typename KeyType, typename ObjectType, typename BuilderType>
void ObjectLibrary<KeyType, ObjectType, BuilderType>::clear()
{
  object_map_.clear();
  meta_map_.clear();
}

template<typename KeyType, typename ObjectType, typename BuilderType>
bool ObjectLibrary<KeyType, ObjectType, BuilderType>::contains(const KeyType& key) const
{
  return meta_map_.find(key) != meta_map_.end();
}

template<typename KeyType, typename ObjectType, typename BuilderType>
bool ObjectLibrary<KeyType, ObjectType, BuilderType>::pending(const KeyType& key) const
{
  return meta_map_.at(key).pending;
}

template<typename KeyType, typename ObjectType, typename BuilderType>
bool ObjectLibrary<KeyType, ObjectType, BuilderType>::pending() const
{
  return std::find_if(meta_map_.begin(), meta_map_.end(),
                      [] (const auto& map_pair) {
                        return map_pair.second.pending;
                      }) != meta_map_.end();
}

#endif//GMetronome_ObjectLibrary_h
