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

#ifndef GMetronome_SpinLock_h
#define Gmetronome_SpinLock_h

#include <atomic>

class SpinLock
{
public:
  void lock() noexcept
    { while (flag_.test_and_set()); }

  void unlock() noexcept
    { flag_.clear(); }
  
  bool try_lock() noexcept
    { return ! flag_.test_and_set(); }

private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

#endif//Gmetronome_SpinLock_h
