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

#include "AudioBackendDummy.h"
#include <thread>

namespace audio {

  namespace {
    
    class TransitionError : public BackendError {
    public:
      TransitionError(BackendState state)
	: BackendError(settings::kAudioBackendNone, state, "invalid state transition")
      {}
    };
    
  }//unnamed namespace
  
  DummyBackend::DummyBackend(const audio::SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec)
  {}

  void DummyBackend::configure(const SampleSpec& spec)
  {
    spec_ = spec;
  }

  void DummyBackend::open()
  {
    if (state_ == BackendState::kConfig)
      state_ = BackendState::kOpen;
    else
      throw TransitionError(state_);
  }

  void DummyBackend::close()
  {
    if (state_ == BackendState::kOpen)
      state_ = BackendState::kConfig;
    else
      throw TransitionError(state_);
  }

  void DummyBackend::start()
  {
    if (state_ == BackendState::kOpen)
      state_ = BackendState::kRunning;
    else
      throw TransitionError(state_);
  }

  void DummyBackend::stop()
  {
    if (state_ == BackendState::kRunning)
      state_ = BackendState::kOpen;
    else
      throw TransitionError(state_);
  }

  void DummyBackend::write(const void* data, size_t bytes)
  {
    if (state_ == BackendState::kRunning)
    {
      if ( bytes > 0 )
      {
        std::this_thread::sleep_for( audio::bytesToUsecs(bytes, spec_) );
      }
    }
    else
      throw TransitionError(state_);
  }

  void DummyBackend::flush() {}

  void DummyBackend::drain() {}

  BackendState DummyBackend::state() const
  {
    return state_;
  }

}//namespace audio
