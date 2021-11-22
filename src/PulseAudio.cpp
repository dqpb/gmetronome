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

#include "PulseAudio.h"
#include "config.h"
#include <iostream>

namespace audio {

  namespace {

    class PulseaudioError : public BackendError {
    public:
      PulseaudioError(BackendState state, const char* what = "")
	: BackendError(settings::kAudioBackendPulseaudio, state, what)
      {}
      PulseaudioError(BackendState state, int error)
	: BackendError(settings::kAudioBackendPulseaudio, state, pa_strerror(error))
      {}
    };
    
    class TransitionError : public PulseaudioError {
    public:
      TransitionError(BackendState state)
	: PulseaudioError(state, "invalid state transition")
      {}
    };

    // Helper
    pa_sample_spec convertSpecToPA(const SampleSpec& spec)
    {
      pa_sample_spec pa_spec;
      pa_spec.rate = spec.rate;
      pa_spec.channels = spec.channels;
  
      switch(spec.format) {
      case SampleFormat::U8        : pa_spec.format = PA_SAMPLE_U8;    break;
      case SampleFormat::ALAW      : pa_spec.format = PA_SAMPLE_ALAW;  break;
      case SampleFormat::ULAW      : pa_spec.format = PA_SAMPLE_ULAW;  break;
      case SampleFormat::S16LE     : pa_spec.format = PA_SAMPLE_S16LE; break;
      case SampleFormat::S16BE     : pa_spec.format = PA_SAMPLE_S16BE; break;
      case SampleFormat::Float32LE : pa_spec.format = PA_SAMPLE_FLOAT32LE; break;
      case SampleFormat::Float32BE : pa_spec.format = PA_SAMPLE_FLOAT32BE; break;
      case SampleFormat::S32LE     : pa_spec.format = PA_SAMPLE_S32LE; break;
      case SampleFormat::S32BE     : pa_spec.format = PA_SAMPLE_S32BE; break;
      case SampleFormat::S24LE     : pa_spec.format = PA_SAMPLE_S24LE; break;
      case SampleFormat::S24BE     : pa_spec.format = PA_SAMPLE_S24BE; break;
      case SampleFormat::S24_32LE  : pa_spec.format = PA_SAMPLE_S32LE; break;
      case SampleFormat::S24_32BE  : pa_spec.format = PA_SAMPLE_S32BE; break;
      default:
	pa_spec.format = PA_SAMPLE_INVALID;
	break;
      };
    
      return pa_spec;
    }

  }//unnamed namespace


  PulseAudioBackend::PulseAudioBackend(const SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec),
      pa_simple_(nullptr)
  {}

  PulseAudioBackend::~PulseAudioBackend()
  {
    if (pa_simple_)
      pa_simple_free(pa_simple_);
  }
  
  void PulseAudioBackend::configure(const SampleSpec& spec)
  {
    spec_ = spec;
  }

  void PulseAudioBackend::open()
  {
    if (state_ != BackendState::kConfig)
      throw TransitionError(state_);
    
    pa_spec_ = convertSpecToPA(spec_);
    
    //pa_buffer_attr_.maxlength = 7056;
    pa_buffer_attr_.maxlength = spec_.channels * 8056;
    pa_buffer_attr_.tlength   = -1;
    pa_buffer_attr_.prebuf    = -1;
    pa_buffer_attr_.minreq    = -1;
    pa_buffer_attr_.fragsize  = -1;
    
    state_ = BackendState::kOpen;
  }
  
  void PulseAudioBackend::close()
  {
    if (state_ != BackendState::kOpen)
      throw TransitionError(state_);

    state_ = BackendState::kConfig;
  }
  
  void PulseAudioBackend::start()
  {
    if (state_ != BackendState::kOpen)
      throw TransitionError(state_);
        
    int error;
    pa_simple_ = pa_simple_new( NULL,
                                PACKAGE_NAME,
                                PA_STREAM_PLAYBACK,
                                NULL,
                                "playback",
                                &pa_spec_,
                                NULL,
                                &pa_buffer_attr_,
                                &error);
    if (!pa_simple_)
      throw PulseaudioError(state_, error);
    
    state_ = BackendState::kRunning;
  }
  
  void PulseAudioBackend::stop()
  {
    if (state_ != BackendState::kRunning)
      throw TransitionError(state_);
    
    if (pa_simple_)
    {
      pa_simple_free(pa_simple_);
      pa_simple_ = nullptr;
    }
    
    state_ = BackendState::kOpen;
  }
  
  void PulseAudioBackend::write(const void* data, size_t bytes)
  {
    // if ( state_ != BackendState::kRunning )
    //   throw TransitionError(state_);
    
    int error;
    if (pa_simple_write(pa_simple_, data, bytes, &error) < 0)
      throw PulseaudioError(state_, error);
  }

  void PulseAudioBackend::flush()
  {
    int error;
    if (pa_simple_flush(pa_simple_, &error) < 0)
      throw PulseaudioError(state_, error);
  }
  
  void PulseAudioBackend::drain()
  {
    int error;
    if (pa_simple_drain(pa_simple_, &error) < 0)
      throw PulseaudioError(state_, error);
  }
  
  uint64_t PulseAudioBackend::latency()
  {
    if ( ! pa_simple_ )
      return 0;
    
    int error;
    uint64_t latency = pa_simple_get_latency(pa_simple_, &error);

    if (error != PA_OK) {
      // throw BackendError( pa_strerror(error) );
    }    
    return latency;
  }

  BackendState PulseAudioBackend::state() const
  {
    return state_;
  }

}//namespace audio
