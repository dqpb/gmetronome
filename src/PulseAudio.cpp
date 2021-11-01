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
#include <iostream>

namespace audio {

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
  
  PulseAudioConnection::PulseAudioConnection(const SampleSpec& spec)
  {
    pa_spec_ = convertSpecToPA(spec);

    //pa_buffer_attr_.maxlength = 7056;
    pa_buffer_attr_.maxlength = 8056;
    pa_buffer_attr_.tlength   = -1;
    pa_buffer_attr_.prebuf    = -1;
    pa_buffer_attr_.minreq    = -1;
    pa_buffer_attr_.fragsize  = -1;
    
    int error;
    pa_simple_ = pa_simple_new( NULL,
                                "gmetronome",
                                PA_STREAM_PLAYBACK,
                                NULL,
                                "playback",
                                &pa_spec_,
                                NULL,
                                &pa_buffer_attr_,
                                &error);
    if (!pa_simple_)
      throw PulseAudioError(error);
  }
  
  PulseAudioConnection::~PulseAudioConnection()
  {
    if (pa_simple_)
      pa_simple_free(pa_simple_);
  }

  PulseAudioConnection::PulseAudioConnection(PulseAudioConnection&& pac)
  {
    pa_spec_ = pac.pa_spec_;
    pa_simple_ = pac.pa_simple_;
    pa_sample_spec_init(&pac.pa_spec_);
    pac.pa_simple_ = nullptr;
  }

  void PulseAudioConnection::write(const void* data, size_t bytes)
  {
    int error;
    if (pa_simple_write(pa_simple_, data, bytes, &error) < 0)
      throw PulseAudioError(error);
  }

  void PulseAudioConnection::flush()
  {
    int error;
    if (pa_simple_flush(pa_simple_, &error) < 0)
      throw PulseAudioError(error);    
  }

  void PulseAudioConnection::drain()
  {
    int error;
    if (pa_simple_drain(pa_simple_, &error) < 0)
      throw PulseAudioError(error);    
  }

  uint64_t PulseAudioConnection::latency()
  {
    int error;
    uint64_t latency = pa_simple_get_latency(pa_simple_, &error);

    if (error != PA_OK) {
      //   throw PulseAudioError(error);
    }    
    return latency;
  }


  PulseAudioSink::PulseAudioSink(const SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec)
  {}
  
  void PulseAudioSink::configure(const SampleSpec& spec)
  {
    spec_ = spec;
  }

  void PulseAudioSink::open()
  {
    if (state_ == BackendState::kConfig)
      // TODO: check configuration
      state_ = BackendState::kOpen;
  }
  
  void PulseAudioSink::close()
  {
    if (state_ == BackendState::kOpen)
      state_ = BackendState::kConfig;
  }
  
  void PulseAudioSink::start()
  {
    if (state_ == BackendState::kOpen)
    {
      pa_connection_ = std::make_unique<PulseAudioConnection>(spec_);
      state_ = BackendState::kRunning;
    }
  }
  
  void PulseAudioSink::stop()
  {
    if (state_ == BackendState::kRunning)
    {
      pa_connection_ = nullptr;
      state_ = BackendState::kOpen;
    }
  }
  
  void PulseAudioSink::write(const void* data, size_t bytes)
  {
    if (pa_connection_)
      pa_connection_->write(data, bytes);
  }

  void PulseAudioSink::flush()
  {
    if (pa_connection_)
      pa_connection_->flush();
  }
  
  void PulseAudioSink::drain()
  {
    if (pa_connection_)
      pa_connection_->drain();
  }
  
  uint64_t PulseAudioSink::latency()
  {
    if (pa_connection_)
      return pa_connection_->latency();
    else
      return 0;
  }

  BackendState PulseAudioSink::state() const
  {
    return state_;
  }

}//namespace audio
