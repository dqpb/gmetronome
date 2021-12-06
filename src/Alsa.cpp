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

#include "Alsa.h"
#include <iostream>

namespace audio {

  namespace {
    
    class AlsaError : public BackendError {
    public:
      AlsaError(BackendState state, const char* what = "")
	: BackendError(settings::kAudioBackendAlsa, state, what)
      {}
      AlsaError(BackendState state, int error)
	: BackendError(settings::kAudioBackendAlsa, state, snd_strerror(error))
      {}
    };
    
    class TransitionError : public AlsaError {
    public:
      TransitionError(BackendState state)
	: AlsaError(state, "invalid state transition")
      {}
    };
    
    // Helper
    snd_pcm_format_t  convertSampleFormatToAlsa(const SampleFormat& format)
    {
      snd_pcm_format_t alsa_format;
      
      switch(format) {
      case SampleFormat::U8        : alsa_format = SND_PCM_FORMAT_U8;    break;
      case SampleFormat::ALAW      : alsa_format = SND_PCM_FORMAT_A_LAW;  break;
      case SampleFormat::ULAW      : alsa_format = SND_PCM_FORMAT_MU_LAW;  break;
      case SampleFormat::S16LE     : alsa_format = SND_PCM_FORMAT_S16_LE; break;
      case SampleFormat::S16BE     : alsa_format = SND_PCM_FORMAT_S16_BE; break;
      case SampleFormat::Float32LE : alsa_format = SND_PCM_FORMAT_FLOAT_LE; break;
      case SampleFormat::Float32BE : alsa_format = SND_PCM_FORMAT_FLOAT_BE; break;
      case SampleFormat::S32LE     : alsa_format = SND_PCM_FORMAT_S32_LE; break;
      case SampleFormat::S32BE     : alsa_format = SND_PCM_FORMAT_S32_BE; break;
      case SampleFormat::S24LE     : alsa_format = SND_PCM_FORMAT_S24_LE; break;
      case SampleFormat::S24BE     : alsa_format = SND_PCM_FORMAT_S24_BE; break;
      case SampleFormat::S24_32LE  : alsa_format = SND_PCM_FORMAT_S32_LE; break;
      case SampleFormat::S24_32BE  : alsa_format = SND_PCM_FORMAT_S32_BE; break;
      default:
	alsa_format = SND_PCM_FORMAT_UNKNOWN;
	break;
      };
      
      return alsa_format;
    }

    const microseconds kRequiredLatency = 100000us;

  }//unnamed namespace

  
  AlsaBackend::AlsaBackend(const audio::SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec),
      hdl_(nullptr)
  {}

  AlsaBackend::~AlsaBackend()
  {
    if (hdl_)
    {
      if (snd_pcm_state(hdl_) == SND_PCM_STATE_RUNNING)
        snd_pcm_drop(hdl_);
      
      snd_pcm_close(hdl_);
    }
  }

  std::vector<DeviceInfo> AlsaBackend::devices()
  {
    // not implemented yet
    return {};
  }

  void AlsaBackend::configure(const SampleSpec& spec)
  {
    spec_ = spec;
  }
  
  void AlsaBackend::open()
  {
    if ( state_ != BackendState::kConfig )
      throw TransitionError(state_);
    
    static const char *device = "default";
    int error;

    if (hdl_ == nullptr)
    {
      error = snd_pcm_open(&hdl_, device, SND_PCM_STREAM_PLAYBACK, 0);
      if (error < 0)
        throw AlsaError(state_, error);
    }
    
    error = snd_pcm_set_params(hdl_,
			       convertSampleFormatToAlsa(spec_.format),
			       SND_PCM_ACCESS_RW_INTERLEAVED,
			       spec_.channels,
			       spec_.rate,
			       1,
			       kRequiredLatency.count());
    if (error < 0)
    {
      error = snd_pcm_close(hdl_);
      if (error >= 0)
        hdl_ = nullptr;

      throw AlsaError(state_, error);
    }
    
    state_ = BackendState::kOpen;
  }

  void AlsaBackend::close()
  {
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    int error;
    
    error = snd_pcm_close(hdl_);
    if (error < 0)
      throw AlsaError(state_, error);

    hdl_ = nullptr;
    state_ = BackendState::kConfig;
  }

  void AlsaBackend::start()
  {
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    int error;

    error = snd_pcm_prepare(hdl_);
    if (error < 0)
      throw AlsaError(state_, error);

    error = snd_pcm_start(hdl_);
    if (error < 0)
      throw AlsaError(state_, error);

    state_ = BackendState::kRunning;
  }

  void AlsaBackend::stop()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    // wait for all pending frames and then stop the PCM
    snd_pcm_drain(hdl_);
    
    state_ = BackendState::kOpen;
  }

  void AlsaBackend::write(const void* data, size_t bytes)
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    snd_pcm_sframes_t n_data_frames = bytes / frameSize(spec_);
    snd_pcm_sframes_t frames_written;
    
    frames_written = snd_pcm_writei(hdl_, data, n_data_frames);

    if (frames_written < 0)
      frames_written = snd_pcm_recover(hdl_, frames_written, 0);

    if (frames_written < 0)
    {
      throw AlsaError(state_, snd_strerror(frames_written));
    }
    else if (frames_written > 0 && frames_written < n_data_frames)
    {
      std::cerr << "Short write (expected " << n_data_frames
		<< ", wrote " << frames_written << " frames)"
		<< std::endl;
    }
  }

  void AlsaBackend::flush()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);
    
    int error;
    
    error = snd_pcm_drop(hdl_);
    if (error < 0)
      throw AlsaError(state_, error);
  }

  void AlsaBackend::drain()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);
    
    int error;
    
    error = snd_pcm_drain(hdl_);
    if (error < 0)
      throw AlsaError(state_, error);
  }

  microseconds AlsaBackend::latency()
  {
    if (!hdl_) return 0us;
    
    int error;
    snd_pcm_sframes_t delayp;    
    
    error = snd_pcm_delay(hdl_, &delayp);
    
    if (error < 0)
      return kRequiredLatency;
    else
      return framesToUsecs(delayp, spec_);
  }

  BackendState AlsaBackend::state() const
  {
    return state_;
  }
 
}//namespace audio
