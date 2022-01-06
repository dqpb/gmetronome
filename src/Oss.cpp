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

    // Helper
    int convertSampleFormatToOss(const SampleFormat& format)
    {
      int oss_format;

      switch(format) {
      // case SampleFormat::U8        : oss_format = AFMT_U8;    break;
      // case SampleFormat::ALAW      : oss_format = AFMT_A_LAW;  break;
      // case SampleFormat::ULAW      : oss_format = AFMT_MU_LAW;  break;
      case SampleFormat::S16LE     : oss_format = AFMT_S16_LE; break;
      // case SampleFormat::S16BE     : oss_format = AFMT_S16_BE; break;
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

    const char* kDefaultDevice = "/dev/dsp";

    const std::string kOssDeviceName = "/dev/dsp";

    const DeviceInfo kOssDeviceInfo =
    {
      kOssDeviceName,
      "Default Output Device",
      2,
      2,
      2,
      kDefaultRate,
      kDefaultRate,
      kDefaultRate
    };

    const DeviceConfig kOssConfig = { kOssDeviceName, kDefaultSpec };

  }//unnamed namespace

  OssBackend::OssBackend()
    : state_(BackendState::kConfig),
      cfg_(kOssConfig),
      fd_(-1)
  {}

  OssBackend::OssBackend(OssBackend&& backend) noexcept
    : state_ {backend.state_},
      cfg_ {std::move(backend.cfg_)},
      fd_ {backend.fd_}
  {
    backend.state_ = BackendState::kConfig;
    backend.fd_ = -1;
  }

  OssBackend::~OssBackend()
  {
    try {
      closeAudioDevice();
    } catch(...) {}
  }

  OssBackend& OssBackend::operator=(OssBackend&& backend) noexcept
  {
    if (this == &backend)
      return *this;

    try {
      closeAudioDevice();
    } catch(...) {}

    state_ = std::exchange(backend.state_, BackendState::kConfig);
    cfg_ = std::move(backend.cfg_);
    fd_ = std::exchange(backend.fd_, -1);

    return *this;
  }

  std::vector<DeviceInfo> OssBackend::devices()
  {
    // TODO: scan for output audio devices
    return {kOssDeviceInfo};
  }

  void OssBackend::configure(const DeviceConfig& config)
  { cfg_ = config; }

  DeviceConfig OssBackend::configuration()
  { return cfg_; }

  DeviceConfig OssBackend::open()
  {
    assert(state_ == BackendState::kConfig);
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
    return {};
  }

  void OssBackend::close()
  {
    assert(state_ == BackendState::kOpen);
    closeAudioDevice();
    state_ = BackendState::kConfig;
  }

  void OssBackend::start()
  {
    assert(state_ == BackendState::kOpen);

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
    assert(state_ == BackendState::kRunning);
    closeAudioDevice();
    state_ = BackendState::kOpen;
  }

  void OssBackend::write(const void* data, size_t bytes)
  {
    assert(state_ == BackendState::kRunning);
    if (::write (fd_, data, bytes) != (ssize_t) bytes)
      throw OssError(state_, errno);
  }

  void OssBackend::flush()
  {
    assert(state_ == BackendState::kRunning);
    // not implemented yet
  }

  void OssBackend::drain()
  {
    assert(state_ == BackendState::kRunning);
    // not implemented yet
  }

  microseconds OssBackend::latency()
  {
    microseconds r = 0us;

    if (fd_ < 0)
      return r;

    int delay;
    if ( ioctl(fd_, SNDCTL_DSP_GETODELAY, &delay) != -1)
      r = bytesToUsecs(delay, cfg_.spec);

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

    const char* device = cfg_.name.empty() ? kDefaultDevice : cfg_.name.c_str();

    if ((fd_ = ::open (device, O_WRONLY, 0)) == -1)
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
    int in_tmp = convertSampleFormatToOss(cfg_.spec.format);
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
    in_tmp = cfg_.spec.channels;
    out_tmp = in_tmp;

    if (ioctl (fd_, SNDCTL_DSP_CHANNELS, &out_tmp) == -1)
      throw OssError(state_, errno);

    if (out_tmp != in_tmp)
      throw OssError(state_, "audio device does not support the requested number of channels");

    //
    // set sample rate
    //
    in_tmp = cfg_.spec.rate;
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
