/*
 * Copyright (C) 2020 The GMetronome Team
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

#ifndef __PROFILES_IO_BASE_H__
#define __PROFILES_IO_BASE_H__

#include "Profile.h"
#include <vector>
#include <sigc++/sigc++.h>

/**
 * @brief Base class for profile i/o modules.
 *
 * This class provides the generic interface for the implementation of
 * profile storage operations used by @link ProfilesManager.
 */
class ProfilesIOBase {

public:

  virtual ~ProfilesIOBase() {}

  /**
   * Returns an up-to-date list of primers (@link Profile::Primer) of all 
   * stored profiles.  These primers contain i.a. the profile identifier 
   * which can later be used to fully load a specific profile via @link 
   * load @endlink method.
   *
   * @return A vector of profile primers.
   */
  virtual std::vector<Profile::Primer> list() = 0;

  /**
   * Load the profile with the identifier id from the underlying data storage. 
   * A list of valid identifiers can be obtained by using list() method.
   *
   * @param id Identifier of the profile.
   * @return The loaded profile.
   */
  virtual Profile load(Profile::Identifier id) = 0;

  /**
   * Store a profile in the underlying data storage. 
   *
   * @param id The profiles identifier.
   * @param profile The profile to store.
   */
  virtual void store(Profile::Identifier id, const Profile& profile) = 0;

  /**
   * Change the order of the stored profiles.
   *
   * @param  A vector of profile identifiers.
   */
  virtual void reorder(const std::vector<Profile::Identifier>& order) = 0;

  /**
   * Remove a profile from the underlying data storage. 
   *
   * @param profile The profile to delete.
   */
  virtual void remove(Profile::Identifier id) = 0;

  /** 
   * Realize all pending changes. 
   *
   * A concrete implementation of this interface might cache profile changes
   * and update the underlying data storage later. This method forces the 
   * synchronization between the internal module data and the data storage.
   */
  virtual void flush() {};

  /**
   * Implementations of this interface should emit this signal if a modification
   * of profiles in the underlying data storage (e.g. a file modification) has 
   * been detected so that the user (ie. the ProfileManager) can take actions
   * to synchronize with the UI data.
   */
  sigc::signal<void> signal_storage_changed()
  { return signal_storage_changed_; }
  
protected:
  sigc::signal<void> signal_storage_changed_;
};

#endif//__PROFILES_IO_BASE_H__
