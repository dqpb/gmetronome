/*
 * Copyright (C) 2021 The GMetronome Team
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

#include "Oss.h"

namespace audio {

  OssBackend::OssBackend(const audio::SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec)
  {
    // not implemented yet
  }

  OssBackend::~OssBackend()
  {
    // not implemented yet
  }

  void OssBackend::configure(const SampleSpec& spec)
  {
    // not implemented yet
  }
  
  void OssBackend::open()
  {
    // not implemented yet
  }

  void OssBackend::close()
  {
    // not implemented yet
  }

  void OssBackend::start()
  {
    // not implemented yet
  }

  void OssBackend::stop()
  {
    // not implemented yet
  }

  void OssBackend::write(const void* data, size_t bytes)
  {
    // not implemented yet
  }

  void OssBackend::flush()
  {
    // not implemented yet
  }

  void OssBackend::drain()
  {
    // not implemented yet
  }

  microseconds OssBackend::latency()
  {
    return 0us;
  }

  BackendState OssBackend::state() const
  {
    return state_;
  }

}//namespace audio
