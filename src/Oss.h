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

#ifndef GMetronome_Oss_h
#define GMetronome_Oss_h

#include "AudioBackend.h"
#include <sys/soundcard.h>

namespace audio {
  
  /**
   * Open Sound System (OSS) Audio Backend
   */ 
  class OssBackend : public Backend
  {
  public:
    OssBackend(const audio::SampleSpec& spec = kDefaultSpec); 
    ~OssBackend();
    
    void configure(const SampleSpec& spec) override;
    void open() override;
    void close() override;
    void start() override;
    void stop() override;
    void write(const void* data, size_t bytes) override;
    void flush() override;
    void drain() override;
    microseconds latency() override;
    BackendState state() const override;
    
  private:
    BackendState state_;
    audio::SampleSpec spec_;
  };
  
}//namespace audio
#endif//GMetronome_Oss_h
