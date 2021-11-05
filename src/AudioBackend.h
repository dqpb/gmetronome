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
#include "Settings.h"
#include <vector>

namespace audio {
  
  /**
   * @class BackendError
   * @brief A generic backend error.
   */
  class BackendError : public GMetronomeError {
  public:
    BackendError(const std::string& text = "")
      : GMetronomeError(text) {} 
  };
  
  class BackendStateTransitionError : public BackendError {
  public:
    BackendStateTransitionError()
      : BackendError("invalid state transition") {} 
  };
  
  class BackendConfigurationError : public BackendError {
  public:
    BackendConfigurationError()
      : BackendError("invalid configuration") {} 
  };
  
  class BackendWriteError : public BackendError {
  public:
    BackendWriteError()
      : BackendError("could not write data stream") {} 
  };
  
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
   * 1) Config:  This state is used to configure the audio::Backend 
   * 2) Open:    After configuration call open() to check configuration and
   *             open the audio device on success.
   * 3) Running: Call start() to reach the Running mode from the Open mode.
   *             In this state you can use the blocking i/o operations (write).
   *
   * The backend state is changed with the following transitions:
   *
   *   I)   Config  --> Config    [configure()]
   *   II)  Config  --> Open      [open()]
   *   III) Open    --> Running   [start()]
   *   IV)  Running --> Open      [stop()]
   *   V)   Open    --> Config    [close()]
   *
   * All other attempts to change the state result in a BackendStateTransitionError.
   */
  class Backend {
  public:
    virtual ~Backend() {}

    virtual void configure(const SampleSpec& spec) = 0;
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void write(const void* data, size_t bytes) = 0;
    virtual void flush() = 0;
    virtual void drain() = 0;
    virtual uint64_t latency() { return 0; }
    virtual BackendState state() const = 0;
  };
  
  /**
   * @function availableBackends
   *
   * Get a list of available audio backend identifiers that can be instantiated 
   * with createBackend().
   *
   * @return A std::vector of audio backend identifiers.
   */
  const std::vector<settings::AudioBackend>& availableBackends();
  
  /** 
   * @function createBackend
   * @brief  Create a new audio backend. 
   * @param  An audio backend identifier.
   * @return  A pointer to the audio backend object or nullptr on error.
   */
  std::unique_ptr<Backend> createBackend(settings::AudioBackend backend);
  
}//namespace audio
#endif//GMetronome_AudioBackend_h
