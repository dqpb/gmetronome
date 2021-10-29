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

#ifndef __PULSEAUDIO_H__
#define __PULSEAUDIO_H__

#include "AudioBackend.h"
#include <exception>
#include <string>
#include <pulse/sample.h>
#include <pulse/simple.h>
#include <pulse/error.h>

namespace audio {

  /**
   * Exception for PulseAudio connections.
   */
  class PulseAudioError : public std::exception {
  public:
    explicit PulseAudioError(int error) noexcept
      : pa_error_(error) {}
    PulseAudioError(const PulseAudioError&) = default;
    PulseAudioError(PulseAudioError&&) = default;
    PulseAudioError& operator=(const PulseAudioError&) = default;
    PulseAudioError& operator=(PulseAudioError&&) = default;
    ~PulseAudioError() noexcept override {}
    
    const char* what() const noexcept override {
      return pa_strerror(pa_error_);
    }    
  private:
    std::string msg_;
    int pa_error_;
  };
  
  /**
   * PulseAudio connection resource.
   */
  class PulseAudioConnection : public AbstractAudioSink {
  public:
    PulseAudioConnection(const audio::SampleSpec spec);  // may throw PulseAudioError
    ~PulseAudioConnection();
    PulseAudioConnection(PulseAudioConnection&&);
    PulseAudioConnection(const PulseAudioConnection&) = delete;
    PulseAudioConnection& operator=(const PulseAudioConnection&) = delete;      
    
    void write(const void* data, size_t bytes) override;
    void flush() override;
    void drain() override;
    uint64_t latency() override;
    
  private:
    pa_sample_spec pa_spec_;
    pa_buffer_attr pa_buffer_attr_;
    pa_simple*     pa_simple_;
  };
  
}//namespace audio
#endif//__PULSEAUDIO_H__
