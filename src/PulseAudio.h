/*
 * Copyright (C) 2020, 2021 The GMetronome Team
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

#ifndef GMetronome_PulseAudio_h
#define GMetronome_PulseAudio_h

#include "AudioBackend.h"
#include <memory>
#include <pulse/sample.h>
#include <pulse/simple.h>
#include <pulse/error.h>

namespace audio {

  /**
   * PulseAudio Backend
   */
  class PulseAudioBackend : public Backend
  {
  public:
    PulseAudioBackend();
    ~PulseAudioBackend();
    PulseAudioBackend(const PulseAudioBackend&) = delete;
    PulseAudioBackend(PulseAudioBackend&&) noexcept;

    PulseAudioBackend& operator=(const PulseAudioBackend&) = delete;
    PulseAudioBackend& operator=(PulseAudioBackend&&) noexcept;

    std::vector<DeviceInfo> devices() override;
    void configure(const DeviceConfig& config) override;
    DeviceConfig configuration() override;
    DeviceConfig open() override;
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
    audio::DeviceConfig cfg_;
    pa_sample_spec pa_spec_;
    pa_buffer_attr pa_buffer_attr_;
    pa_simple*     pa_simple_;
};

}//namespace audio
#endif//GMetronome_PulseAudio_h
