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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "PulseAudio.h"
#include <cassert>
#include <utility>

#ifndef NDEBUG
#  include <iostream>
#endif

namespace audio {

  namespace {

    class PulseaudioError : public BackendError {
    public:
      explicit PulseaudioError(BackendState state, const char* what = "")
        : BackendError(BackendIdentifier::kPulseAudio, state, what)
      {}
      PulseaudioError(BackendState state, int error)
        : BackendError(BackendIdentifier::kPulseAudio, state, pa_strerror(error))
      {}
    };

    // convert sample formats
    const std::vector<std::pair<SampleFormat, pa_sample_format_t>> kFormatMap =
    {
      {SampleFormat::kU8        , PA_SAMPLE_U8},
      // {SampleFormat::kS8     , <unsupported>},
      {SampleFormat::kS16LE     , PA_SAMPLE_S16LE},
      {SampleFormat::kS16BE     , PA_SAMPLE_S16BE},
      // {SampleFormat::kU16LE  , <unsupported>},
      // {SampleFormat::kU16BE  , <unsupported>},
      {SampleFormat::kS32LE     , PA_SAMPLE_S32LE},
      {SampleFormat::kS32BE     , PA_SAMPLE_S32BE},
      {SampleFormat::kFloat32LE , PA_SAMPLE_FLOAT32LE},
      {SampleFormat::kFloat32BE , PA_SAMPLE_FLOAT32BE},
      // {SampleFormat::kS24LE    , PA_SAMPLE_S24LE},
      // {SampleFormat::kS24BE    , PA_SAMPLE_S24BE},
      // {SampleFormat::kS24_32LE , PA_SAMPLE_S32LE},
      // {SampleFormat::kS24_32BE , PA_SAMPLE_S32BE},
      // {SampleFormat::kALAW     , PA_SAMPLE_ALAW},
      // {SampleFormat::kULAW     , PA_SAMPLE_ULAW},
      {SampleFormat::kUnknown , PA_SAMPLE_INVALID}
    };

    // helper
    pa_sample_format_t formatToPA(const SampleFormat& fmt)
    {
      auto it = std::find_if(kFormatMap.begin(), kFormatMap.end(),
                             [&fmt] (const auto& p) { return p.first == fmt; });

      if (it != kFormatMap.end())
        return it->second;
      else
        return PA_SAMPLE_INVALID;
    }

    SampleFormat formatFromPA(pa_sample_format_t fmt)
    {
      auto it = std::find_if(kFormatMap.begin(), kFormatMap.end(),
                             [&fmt] (const auto& p) { return p.second == fmt; });

      if (it != kFormatMap.end())
        return it->first;
      else
        return SampleFormat::kUnknown;
    }

    pa_sample_spec specToPA(const StreamSpec& spec) noexcept
    {
      pa_sample_spec pa_spec;

      pa_spec.format = formatToPA(spec.format);
      pa_spec.rate = spec.rate;
      pa_spec.channels = spec.channels;

      return pa_spec;
    }

    StreamSpec specFromPA(const pa_sample_spec& pa_spec)
    {
      StreamSpec spec;

      spec.format = formatFromPA(pa_spec.format);
      spec.rate = pa_spec.rate;
      spec.channels = pa_spec.channels;

      return spec;
    };

    const pa_sample_spec kPADefaultSpec = specToPA(kDefaultSpec);

    const DeviceInfo kPADefaultInfo =
    {
      "",
      "Default Output Device",
      kPADefaultSpec.channels,
      kPADefaultSpec.channels,
      kPADefaultSpec.channels,
      kPADefaultSpec.rate,
      kPADefaultSpec.rate,
      kPADefaultSpec.rate
    };

    const pa_buffer_attr kPADefaultBufferAttr =
    {
      (uint32_t) -1, // maxlength
      (uint32_t) -1, // tlength
      (uint32_t) -1, // prebuf
      (uint32_t) -1, // minreq
      (uint32_t) -1  // fragsize
    };

    std::chrono::duration<double> kPAMaxBufferDuration = 90ms;

  }//unnamed namespace


  PulseAudioBackend::PulseAudioBackend()
    : state_ {BackendState::kConfig},
      cfg_ {kDefaultConfig},
      pa_spec_ {kPADefaultSpec},
      pa_buffer_attr_ {kPADefaultBufferAttr},
      pa_simple_ {nullptr}
  {}

  PulseAudioBackend::PulseAudioBackend(PulseAudioBackend&& backend) noexcept
    : state_ { std::move(backend.state_) },
      cfg_ { std::move(backend.cfg_) },
      pa_spec_ { std::move(backend.pa_spec_) },
      pa_buffer_attr_ { std::move(backend.pa_buffer_attr_) },
      pa_simple_ { std::move(backend.pa_simple_) }
  {
    backend.state_ = BackendState::kConfig;
    backend.pa_spec_ = kPADefaultSpec;
    backend.pa_buffer_attr_ = kPADefaultBufferAttr;
    backend.pa_simple_ = nullptr;
  }

  PulseAudioBackend::~PulseAudioBackend()
  {
    if (pa_simple_)
      pa_simple_free(pa_simple_);
  }

  PulseAudioBackend& PulseAudioBackend::operator=(PulseAudioBackend&& backend) noexcept
  {
    if (this == &backend)
      return *this;

    state_ = std::exchange(backend.state_, BackendState::kConfig);

    cfg_ = std::move(backend.cfg_);

    pa_spec_ = std::exchange(backend.pa_spec_, kPADefaultSpec);

    pa_buffer_attr_ = std::exchange(backend.pa_buffer_attr_, kPADefaultBufferAttr);

    if (pa_simple_)
      pa_simple_free(pa_simple_);

    pa_simple_ = std::exchange(backend.pa_simple_, nullptr);

    return *this;
  }

  std::vector<DeviceInfo> PulseAudioBackend::devices()
  {
    // TODO: scan pulseaudio server for sinks
    return {kPADefaultInfo};
  }

  void PulseAudioBackend::configure(const DeviceConfig& config)
  {
    cfg_ = config;
  }

  DeviceConfig PulseAudioBackend::configuration()
  { return cfg_; }

  DeviceConfig PulseAudioBackend::open()
  {
    assert(state_ == BackendState::kConfig);

    pa_spec_ = specToPA(cfg_.spec);

    pa_buffer_attr_ = kPADefaultBufferAttr;
    pa_buffer_attr_.maxlength =
      pa_bytes_per_second(&pa_spec_) * kPAMaxBufferDuration.count();

    state_ = BackendState::kOpen;

    DeviceConfig actual_cfg = cfg_;
    actual_cfg.spec = specFromPA(pa_spec_);

    return actual_cfg;
  }

  void PulseAudioBackend::close()
  {
    assert(state_ == BackendState::kOpen);
    state_ = BackendState::kConfig;
  }

  void PulseAudioBackend::start()
  {
    assert(state_ == BackendState::kOpen);

    const char* dev = cfg_.name.empty() ? NULL : cfg_.name.c_str() ;

    int error;
    pa_simple_ = pa_simple_new( NULL,
                                PACKAGE_NAME,
                                PA_STREAM_PLAYBACK,
                                dev,
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
    assert(state_ == BackendState::kRunning);

    if (pa_simple_)
    {
      int error;
      if (pa_simple_drain(pa_simple_, &error) < 0)
      {
#ifndef NDEBUG
        std::cerr << "PulseBackend: draining failed but will continue to stop the backend"
                  << std::endl;
#endif
      }
      pa_simple_free(pa_simple_);
      pa_simple_ = nullptr;
    }

    state_ = BackendState::kOpen;
  }

  void PulseAudioBackend::write(const void* data, size_t bytes)
  {
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

  microseconds PulseAudioBackend::latency()
  {
    if ( ! pa_simple_ )
      return 0us;

    int error;
    uint64_t latency = pa_simple_get_latency(pa_simple_, &error);

    if (error != PA_OK) {
      // throw BackendError( pa_strerror(error) );
    }
    return microseconds(latency);
  }

  BackendState PulseAudioBackend::state() const
  {
    return state_;
  }

}//namespace audio
