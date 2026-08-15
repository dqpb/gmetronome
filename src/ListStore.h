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
#include <sigc++/sigc++.h>

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

  virtual ~ListStore() {}

  /**
   * Returns an up-to-date list of primers (@link struct Primer) of all
   * stored items.  These primers contain i.a. the identifier which can
   * later be used to fully load a specific item via @link load @endlink.
   *
   * @return A vector of item primers.
   */
  virtual std::vector<Primer> list() = 0;

  /**
   * Load the item with the identifier id from the underlying data storage.
   * A list of valid identifiers can be obtained by using list() method.
   *
   * @param id Identifier of the item.
   * @return The loaded item.
   */
  virtual Type load(Identifier id) = 0;

  /**
   * Store an item in the underlying data storage.
   *
   * @param id The item identifier.
   * @param item The item to store.
   */
  virtual void store(Identifier id, const Type& item) = 0;

  /**
   * Change the order of the stored items.
   *
   * @param  A vector of item identifiers.
   */
  virtual void reorder(const std::vector<Identifier>& order) = 0;

  /**
   * Remove an item from the underlying data storage.
   *
   * @param id The identifier of the item to delete.
   */
  virtual void remove(Identifier id) = 0;

  /**
   * Realize all pending changes.
   *
   * A concrete implementation of this interface might cache item changes
   * and update the underlying data storage later. This method forces the
   * synchronization between the internal module data and the data storage.
   */
  virtual void flush() {};

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
