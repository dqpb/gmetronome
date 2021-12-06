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

#include "Oss.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <cassert>

namespace audio {

  namespace {

    class OssError : public BackendError {
    public:
      OssError(BackendState state, const char* what = "")
        : BackendError(settings::kAudioBackendOss, state, what)
        {}
      OssError(BackendState state, int error)
        : BackendError(settings::kAudioBackendOss, state, strerror(error))
        {}
    };

    class TransitionError : public OssError {
    public:
      TransitionError(BackendState state)
        : OssError(state, "invalid state transition")
        {}
    };

    // Helper
    int convertSampleFormatToOss(const SampleFormat& format)
    {
      int oss_format;

      switch(format) {
      case SampleFormat::U8        : oss_format = AFMT_U8;    break;
      case SampleFormat::ALAW      : oss_format = AFMT_A_LAW;  break;
      case SampleFormat::ULAW      : oss_format = AFMT_MU_LAW;  break;
      case SampleFormat::S16LE     : oss_format = AFMT_S16_LE; break;
      case SampleFormat::S16BE     : oss_format = AFMT_S16_BE; break;
      // case SampleFormat::S32LE     : oss_format = AFMT_S32_LE; break;
      // case SampleFormat::S32BE     : oss_format = AFMT_S32_BE; break;
      // case SampleFormat::S24LE     : oss_format = AFMT_S24_LE; break;
      // case SampleFormat::S24BE     : oss_format = AFMT_S24_BE; break;
      default:
        oss_format = -1;
        break;
      };

      return oss_format;
    }

    const char* kDeviceName = "/dev/dsp";

  }//unnamed namespace

  OssBackend::OssBackend(const audio::SampleSpec& spec)
    : state_(BackendState::kConfig),
      spec_(spec),
      fd_(-1)
  {}

  OssBackend::~OssBackend()
  {
    try {
      closeAudioDevice();
    } catch(...) {}
  }

  std::vector<DeviceInfo> OssBackend::devices()
  {
    // not implemented yet
    return {};
  }

  void OssBackend::configure(const SampleSpec& spec)
  {
    spec_ = spec;
  }

  void OssBackend::open()
  {
    if ( state_ != BackendState::kConfig )
      throw TransitionError(state_);

    openAudioDevice();

    try {
      configureAudioDevice();
    }
    catch(...)
    {
      try { closeAudioDevice(); } catch(...) {}
      throw;
    }

    state_ = BackendState::kOpen;
  }

  void OssBackend::close()
  {
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    closeAudioDevice();

    state_ = BackendState::kConfig;
  }

  void OssBackend::start()
  {
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    // since the device might have been closed in a previous stop() call
    // we need to re-open and re-configure it
    if (fd_ < 0)
    {
      openAudioDevice();

      try {
        configureAudioDevice();
      }
      catch(...)
      {
        try { closeAudioDevice(); } catch(...) {}
        throw;
      }
    }

    state_ = BackendState::kRunning;
  }

  void OssBackend::stop()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    closeAudioDevice();

    state_ = BackendState::kOpen;
  }

  void OssBackend::write(const void* data, size_t bytes)
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    if (::write (fd_, data, bytes) != bytes)
      throw OssError(state_, errno);
  }

  void OssBackend::flush()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    // not implemented yet
  }

  void OssBackend::drain()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    // not implemented yet
  }

  microseconds OssBackend::latency()
  {
    microseconds r = 0us;

    if (fd_ < 0)
      return r;

    int delay;
    if (ioctl(fd_, SNDCTL_DSP_GETODELAY, &delay) != -1)
      r = bytesToUsecs(delay,spec_);

    return r;
  }

  BackendState OssBackend::state() const
  {
    return state_;
  }

  void OssBackend::openAudioDevice()
  {
    if (fd_ >= 0)
      return;

    if ((fd_ = ::open (kDeviceName, O_WRONLY, 0)) == -1)
      throw OssError(state_, "failed to open audio device");
  }

  void OssBackend::configureAudioDevice()
  {
    if (fd_ < 0)
      return;

    //
    // set buffer size
    //
    int max_fragments = 16;
    int size_selector = 8;
    int frag = (max_fragments << 16) | (size_selector);

    if (ioctl(fd_, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
      throw OssError(state_, errno);

    //
    // set sample format
    //
    int in_tmp = convertSampleFormatToOss(spec_.format);
    int out_tmp = in_tmp;

    if (in_tmp < 0)
      throw OssError(state_, "failed to convert sample format");

    if (ioctl (fd_, SNDCTL_DSP_SETFMT, &out_tmp) == -1)
      throw OssError(state_, errno);

    if (out_tmp != in_tmp)
      throw OssError(state_, "audio device does not support the requested sample format");

    //
    // set numer of channels
    //
    in_tmp = spec_.channels;
    out_tmp = in_tmp;

    if (ioctl (fd_, SNDCTL_DSP_CHANNELS, &out_tmp) == -1)
      throw OssError(state_, errno);

    if (out_tmp != in_tmp)
      throw OssError(state_, "audio device does not support the requested number of channels");

    //
    // set sample rate
    //
    in_tmp = spec_.rate;
    out_tmp = in_tmp;

    if (ioctl (fd_, SNDCTL_DSP_SPEED, &out_tmp) == -1)
      throw OssError(state_, errno);

    if (out_tmp != in_tmp)
      throw OssError(state_, "audio device does not support the requested sample rate");
  }

  void OssBackend::closeAudioDevice()
  {
    if (fd_ < 0)
      return;

    if ((fd_ = ::close(fd_)) == -1)
      throw OssError(state_, errno);

    fd_ = -1;
  }

}//namespace audio
