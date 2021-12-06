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

#ifndef GMetronome_AudioBackendDummy_h
#define GMetronome_AudioBackendDummy_h

#include "AudioBackend.h"

namespace audio {

  class DummyBackend : public Backend{

  public:
    DummyBackend(const audio::SampleSpec& spec = kDefaultSpec);

    std::vector<DeviceInfo> devices() override;
    void configure(const SampleSpec& spec) override;
    void open() override;
    void close() override;
    void start() override;
    void stop() override;
    void write(const void* data, size_t bytes) override;
    void flush() override;
    void drain() override;
    BackendState state() const override;

  private:
    BackendState state_;
    SampleSpec spec_;
  };
  
}//namespace audio
#endif//GMetronome_AudioBackendDummy_h
