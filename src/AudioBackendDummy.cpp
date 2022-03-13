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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "AudioBackendDummy.h"
#include <thread>
#include <cassert>

namespace audio {

  namespace {

    const std::string kDummyDeviceName = ""; // Default

    const DeviceInfo kDummyInfo =
    {
      kDummyDeviceName,
      "No Audio Output",
      2,
      2,
      2,
      kDefaultRate,
      kDefaultRate,
      kDefaultRate
    };

    const DeviceConfig kDummyConfig = { kDummyDeviceName, kDefaultSpec };

  }//unnamed namespace

  DummyBackend::DummyBackend()
    : state_(BackendState::kConfig),
      cfg_(kDummyConfig)
  {}

  std::vector<DeviceInfo> DummyBackend::devices()
  { return {kDummyInfo}; }

  void DummyBackend::configure(const DeviceConfig& config)
  { cfg_ = config; }

  DeviceConfig DummyBackend::configuration()
  { return cfg_; }

  DeviceConfig DummyBackend::open()
  {
    assert(state_ == BackendState::kConfig);
    state_ = BackendState::kOpen;
    return kDummyConfig;
  }

  void DummyBackend::close()
  {
    assert(state_ == BackendState::kOpen);
    state_ = BackendState::kConfig;
  }

  void DummyBackend::start()
  {
    assert(state_ == BackendState::kOpen);
    state_ = BackendState::kRunning;
  }

  void DummyBackend::stop()
  {
    assert(state_ == BackendState::kRunning);
    state_ = BackendState::kOpen;
  }

  void DummyBackend::write(const void* data, size_t bytes)
  {
    assert(state_ == BackendState::kRunning);
    if ( bytes > 0 )
      std::this_thread::sleep_for( audio::bytesToUsecs(bytes, kDummyConfig.spec) );
  }

  void DummyBackend::flush() {}

  void DummyBackend::drain() {}

  BackendState DummyBackend::state() const
  {
    return state_;
  }

}//namespace audio
