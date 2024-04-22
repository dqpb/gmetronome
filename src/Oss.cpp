/*
 * Copyright (C) 2021, 2022 The GMetronome Team
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Oss.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <cassert>
#include <cstring>
#include <utility>

#ifndef NDEBUG
#  include <iostream>
#endif

namespace audio {

  namespace {

    class OssError : public BackendError {
    public:
      explicit OssError(BackendState state, const char* what = "")
        : BackendError(BackendIdentifier::kOSS, state, what)
        {}
      OssError(BackendState state, int error)
        : BackendError(BackendIdentifier::kOSS, state, std::strerror(error))
        {}
      OssError(BackendState state, const std::string& what, int error)
        : BackendError(BackendIdentifier::kOSS, state,
                       what + " (" + std::to_string(error) +
                       " '" + std::string(std::strerror(error)) + "')")
        {}
    };

    constexpr int kUnknownFormat = -1;

    // convert sample formats
    const std::vector<std::pair<SampleFormat, int>> kFormatMap =
    {
      {SampleFormat::kU8        , AFMT_U8},
      {SampleFormat::kS8        , AFMT_S8},
      {SampleFormat::kS16LE     , AFMT_S16_LE},
      {SampleFormat::kS16BE     , AFMT_S16_BE},
      {SampleFormat::kU16LE     , AFMT_U16_LE},
      {SampleFormat::kU16BE     , AFMT_U16_BE},
      // {SampleFormat::kS32LE     , AFMT_S32_LE},
      // {SampleFormat::kS32BE     , AFMT_S32_BE},
      // {SampleFormat::kFloat32LE , AFMT_},
      // {SampleFormat::kFloat32BE , AFMT_},
      // {SampleFormat::kS24LE     , AFMT_},
      // {SampleFormat::kS24BE     , AFMT_},
      // {SampleFormat::kS24_32LE  , AFMT_},
      // {SampleFormat::kS24_32BE  , AFMT_},
      // {SampleFormat::kALAW      , AFMT_},
      // {SampleFormat::kULAW      , AFMT_},
      {SampleFormat::kUnknown   , kUnknownFormat}
    };

    // helper
    int formatToOss(const SampleFormat& fmt)
    {
      auto it = std::find_if(kFormatMap.begin(), kFormatMap.end(),
                             [&fmt] (const auto& p) { return p.first == fmt; });

      if (it != kFormatMap.end())
        return it->second;
      else
        return kUnknownFormat;
    }

    SampleFormat formatFromOss(int fmt)
    {
      auto it = std::find_if(kFormatMap.begin(), kFormatMap.end(),
                             [&fmt] (const auto& p) { return p.second == fmt; });

      if (it != kFormatMap.end())
        return it->first;
      else
        return SampleFormat::kUnknown;
    }

    const char* kOssDefaultDevice = "/dev/dsp";

    const std::string kOssDefaultDeviceName = "/dev/dsp";

    const DeviceInfo kOssDefaultDeviceInfo =
    {
      kOssDefaultDeviceName,
      "Default Output Device",
      kDefaultChannels,
      kDefaultChannels,
      kDefaultChannels,
      kDefaultRate,
      kDefaultRate,
      kDefaultRate
    };

    const DeviceConfig kOssDefaultConfig = { kOssDefaultDeviceName, kDefaultSpec };

  }//unnamed namespace

  OssBackend::OssBackend()
    : state_(BackendState::kConfig),
      in_cfg_(kOssDefaultConfig),
      out_cfg_(kOssDefaultConfig),
      fd_(-1)
  {}

  OssBackend::OssBackend(OssBackend&& backend) noexcept
    : state_ {backend.state_},
      in_cfg_ {std::move(backend.in_cfg_)},
      out_cfg_ {std::move(backend.out_cfg_)},
      fd_ {backend.fd_}
  {
    backend.state_ = BackendState::kConfig;
    backend.fd_ = -1;
  }

  OssBackend::~OssBackend()
  {
    try {
      closeDevice();
    }
    catch(const OssError& e)
    {
#ifndef NDEBUG
      std::cerr << "OssBackend: " << e.what() << std::endl;
#endif
    }
  }

  OssBackend& OssBackend::operator=(OssBackend&& backend) noexcept
  {
    if (this == &backend)
      return *this;

    try {
      closeDevice();
    }
    catch(const OssError& e)
    {
#ifndef NDEBUG
      std::cerr << "OssBackend: " << e.what() << std::endl;
#endif
    }
    state_ = std::exchange(backend.state_, BackendState::kConfig);
    in_cfg_ = std::move(backend.in_cfg_);
    out_cfg_ = std::move(backend.out_cfg_);
    fd_ = std::exchange(backend.fd_, -1);

    return *this;
  }

  std::vector<DeviceInfo> OssBackend::devices()
  {
    // TODO: scan for output audio devices
    return {kOssDefaultDeviceInfo};
  }

  void OssBackend::configure(const DeviceConfig& config)
  { in_cfg_ = config; }

  DeviceConfig OssBackend::configuration()
  { return in_cfg_; }

  DeviceConfig OssBackend::open()
  {
    assert(state_ == BackendState::kConfig);
    openAndConfigureDevice(); // updates out_cfg_
    state_ = BackendState::kOpen;
    return out_cfg_;
  }

  void OssBackend::close()
  {
    assert(state_ == BackendState::kOpen);
    closeDevice();
    state_ = BackendState::kConfig;
  }

  void OssBackend::start()
  {
    assert(state_ == BackendState::kOpen);

    // The device might have been closed in a previous stop() call
    // so we need to re-open and re-configure it
    if (fd_ < 0)
      openAndConfigureDevice();

    state_ = BackendState::kRunning;
  }

  void OssBackend::stop()
  {
    assert(state_ == BackendState::kRunning);
    closeDevice();
    state_ = BackendState::kOpen;
  }

  void OssBackend::write(const void* data, size_t bytes)
  {
    assert(state_ == BackendState::kRunning);
    if (::write (fd_, data, bytes) != (ssize_t) bytes)
      throw OssError(state_, "write failed", errno);
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
      r = bytesToUsecs(delay, out_cfg_.spec);

    return r;
  }

  BackendState OssBackend::state() const
  {
    return state_;
  }

  void OssBackend::openDevice()
  {
    if (fd_ >= 0)
      return;

    const char* device = in_cfg_.name.empty() ? kOssDefaultDevice : in_cfg_.name.c_str();

    if ((fd_ = ::open (device, O_WRONLY, 0)) == -1)
      throw OssError(state_, "failed to open audio device", errno);

    out_cfg_.name = device;
  }

  void OssBackend::closeDevice()
  {
    if (fd_ < 0)
      return;

    if ((fd_ = ::close(fd_)) == -1)
      throw OssError(state_, "failed to close audio device", errno);

    fd_ = -1;
  }

  void OssBackend::configureDevice()
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
      throw OssError(state_, "failed to set buffer size", errno);

    //
    // set sample format
    //
    int fmt = formatToOss(in_cfg_.spec.format);

    if (fmt < 0)
      throw OssError(state_, "invalid or unsupported sample format");

    if (ioctl (fd_, SNDCTL_DSP_SETFMT, &fmt) == -1)
      throw OssError(state_, "failed to set sample format", errno);

    out_cfg_.spec.format = formatFromOss(fmt); // set actual format

    //
    // set numer of channels
    //
    unsigned int channels = in_cfg_.spec.channels;

    if (ioctl (fd_, SNDCTL_DSP_CHANNELS, &channels) == -1)
      throw OssError(state_, "failed to set channel number", errno);

    out_cfg_.spec.channels = channels; // set actual number of channels

    //
    // set sample rate
    //
    int speed = in_cfg_.spec.rate;

    if (ioctl (fd_, SNDCTL_DSP_SPEED, &speed) == -1)
      throw OssError(state_, "failed to set sample rate", errno);

    out_cfg_.spec.rate = speed;
  }

  void OssBackend::openAndConfigureDevice()
  {
    openDevice();
    try {
      configureDevice();
    }
    catch(...)
    {
#ifndef NDEBUG
      std::cerr << "OssBackend: closing device after error during device configuration"
                << std::endl;
#endif
      try {
        closeDevice();
      }
      catch(const OssError& e)
      {
#ifndef NDEBUG
        std::cerr << "OssBackend: " << e.what() << std::endl;
#endif
      }
      throw;
    }
  }

}//namespace audio
