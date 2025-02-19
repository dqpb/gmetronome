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

#ifndef GMetronome_AudioBackend_h
#define GMetronome_AudioBackend_h

#include "Audio.h"
#include "Error.h"
#include <vector>
#include <memory>

namespace audio {

  /** A structure providing information and capabilities of playback devices. */
  struct DeviceInfo
  {
    std::string    name;          //!< Unique name of the device
    std::string    descr;         //!< Device description
    int            min_channels;  //!< Minimum number of channels
    int            max_channels;  //!< Maximum number of channels
    int            channels;      //!< Preferred number of channels
    SampleRate     min_rate;      //!< Minimum sample rate
    SampleRate     max_rate;      //!< Maximum sample rate
    SampleRate     rate;          //!< Preferred sample rate
  };

  /** A structure to configure an audio device. */
  struct DeviceConfig
  {
    std::string name;
    StreamSpec  spec;
  };

  const DeviceConfig kDefaultConfig = { "", kDefaultSpec };

  enum class BackendState
  {
    kConfig   = 0,
    kOpen     = 1,
    kRunning  = 2
  };

  /**
   * @class Backend
   *
   * An audio backend is always in one of the following states:
   *
   * -# Config:  This state is used to configure the audio::Backend
   * -# Open:    After configuration use open() to check the configuration and
   *             open the audio device.
   * -# Running: Call start() to reach the Running mode from the Open mode.
   *             In this state you can use the blocking i/o operations (write).
   *
   * The backend state is changed with the following transitions:
   *
   *   -# Config  --> Config    [@ref configure()]
   *   -# Config  --> Open      [@ref open()]
   *   -# Open    --> Running   [@ref start()]
   *   -# Running --> Open      [@ref stop()]
   *   -# Open    --> Config    [@ref close()]

   * All other attempts to change the state result in a state transition error.
   */
  class Backend {
  public:
    virtual ~Backend() {}

    virtual std::vector<DeviceInfo> devices() = 0;
    virtual void configure(const DeviceConfig& config) = 0;
    virtual DeviceConfig configuration() = 0;
    virtual DeviceConfig open() = 0;
    virtual void close() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void write(const void* data, size_t bytes) = 0;
    virtual void flush() = 0;
    virtual void drain() = 0;
    virtual microseconds latency() { return 0us; }
    virtual BackendState state() const = 0;
  };

  enum class BackendIdentifier
  {
    kNone = 0,
#if HAVE_ALSA
    kALSA = 1,
#endif
#if HAVE_OSS
    kOSS = 2,
#endif
#if HAVE_PULSEAUDIO
    kPulseAudio = 3,
#endif
  };

  /**
   * Get a list of available audio backend identifiers that can be instantiated
   * with createBackend().
   *
   * @return A std::vector of audio backend identifiers.
   */
  const std::vector<BackendIdentifier>& availableBackends();

  /**
   * @brief  Create a new audio backend.
   * @param  An audio backend identifier.
   * @return  A pointer to the audio backend object or nullptr on error.
   */
  std::unique_ptr<Backend> createBackend(BackendIdentifier backend);

  /**
   * @class BackendError
   * @brief A generic error for audio backends
   */
  class BackendError : public GMetronomeError {
  public:
    BackendError(BackendIdentifier backend, BackendState state, const std::string& what = "")
      : GMetronomeError(what),
        backend_(backend),
        state_(state)
    {}

    BackendIdentifier backend() const noexcept
    { return backend_; }

    BackendState state() const noexcept
    { return state_; }

  private:
    BackendIdentifier backend_;
    BackendState state_;
  };

}//namespace audio
#endif//GMetronome_AudioBackend_h
