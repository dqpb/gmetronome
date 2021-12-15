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
#include <alsa/asoundlib.h>
#include <memory>
#include <algorithm>
#include <map>
#include <thread>
#include <iostream>

namespace audio {

  const microseconds kRequiredLatency    = 80ms;

  class AlsaError : public GMetronomeError {
  public:
    AlsaError(const std::string& msg = "", int error = 0)
      : GMetronomeError {msg},
        error_(error) {}
    int alsaErrorCode() const
      {return error_;}
    const char* alsaErrorString() const
      { return (error_ < 0 ? snd_strerror (error_) : "unknown error"); }
  private:
    int error_;
  };

  class AlsaBackendError : public BackendError {
  public:
    AlsaBackendError(BackendState state, const std::string& what = "", int error = 0)
      : BackendError(settings::kAudioBackendAlsa, state, what),
        error_(error)
      {}
    AlsaBackendError(BackendState state, int error, const std::string& what = "")
      : AlsaBackendError(state, what, error)
      {}
  private:
    int error_;
  };

  class TransitionError : public AlsaBackendError {
  public:
    TransitionError(BackendState state)
      : AlsaBackendError(state, "invalid state transition")
      {}
  };

  // helper to create AlsaBackendError from AlsaError
  AlsaBackendError makeAlsaBackendError(BackendState state, const AlsaError& e)
  {
    auto msg = std::string(e.what())
      + " (" + std::to_string(e.alsaErrorCode()) + " '" + std::string(e.alsaErrorString()) + "')";
#ifndef NDEBUG
    std::cerr << "AlsaBackend: " << msg << std::endl;
#endif
    return AlsaBackendError (state, msg, e.alsaErrorCode());
  }

  snd_pcm_format_t  convertSampleFormatToAlsa(const SampleFormat& format)
  {
    snd_pcm_format_t alsa_format;

    switch(format) {
    // case SampleFormat::U8        : alsa_format = SND_PCM_FORMAT_U8; break;
    // case SampleFormat::ALAW      : alsa_format = SND_PCM_FORMAT_A_LAW; break;
    // case SampleFormat::ULAW      : alsa_format = SND_PCM_FORMAT_MU_LAW; break;
    case SampleFormat::S16LE     : alsa_format = SND_PCM_FORMAT_S16_LE; break;
    // case SampleFormat::S16BE     : alsa_format = SND_PCM_FORMAT_S16_BE; break;
    // case SampleFormat::Float32LE : alsa_format = SND_PCM_FORMAT_FLOAT_LE; break;
    // case SampleFormat::Float32BE : alsa_format = SND_PCM_FORMAT_FLOAT_BE; break;
    // case SampleFormat::S32LE     : alsa_format = SND_PCM_FORMAT_S32_LE; break;
    // case SampleFormat::S32BE     : alsa_format = SND_PCM_FORMAT_S32_BE; break;
    // case SampleFormat::S24LE     : alsa_format = SND_PCM_FORMAT_S24_LE; break;
    // case SampleFormat::S24BE     : alsa_format = SND_PCM_FORMAT_S24_BE; break;
    // case SampleFormat::S24_32LE  : alsa_format = SND_PCM_FORMAT_S32_LE; break;
    // case SampleFormat::S24_32BE  : alsa_format = SND_PCM_FORMAT_S32_BE; break;
    default:
      alsa_format = SND_PCM_FORMAT_UNKNOWN;
      break;
    };

    return alsa_format;
  }

  struct AlsaDeviceDescription
  {
    std::string name;
    std::string descr;
  };

  struct AlsaDeviceCaps
  {
    std::vector<snd_pcm_format_t> formats;
    unsigned int min_channels {0};
    unsigned int max_channels {0};
    unsigned int min_rate {0};
    unsigned int max_rate {0};
    unsigned int min_periods {0};
    unsigned int max_periods {0};
    snd_pcm_uframes_t min_period_size {0};
    snd_pcm_uframes_t max_period_size {0};
    snd_pcm_uframes_t min_buffer_size {0};
    snd_pcm_uframes_t max_buffer_size {0};
  };

  [[maybe_unused]] std::ostream& operator<<(std::ostream& os, const AlsaDeviceCaps& dev)
  {
    os << "Channels    : ["
       << dev.min_channels << "," << dev.max_channels << "]" << std::endl
       << "Rate        : ["
       << dev.min_rate << "," << dev.max_rate << "]" << std::endl
       << "Periods     : ["
       << dev.min_periods << "," << dev.max_periods << "]" << std::endl
       << "Period Size : ["
       << dev.min_period_size << "," << dev.max_period_size << "]" << std::endl
       << "Buffer Size : ["
       << dev.min_buffer_size << "," << dev.max_buffer_size << "]" << std::endl;
    return  os;
  }

  struct AlsaDeviceConfig
  {
    snd_pcm_format_t format;
    unsigned int channels;
    unsigned int rate;
    unsigned int periods;
    snd_pcm_uframes_t period_size;
  };

  [[maybe_unused]] std::ostream& operator<<(std::ostream& os, const AlsaDeviceConfig& cfg)
  {
    os << "["
       << snd_pcm_format_name(cfg.format) << ", "
       << cfg.channels << ", "
       << cfg.rate << ", "
       << cfg.periods << ", "
       << cfg.period_size << "]";
    return  os;
  }

  // debug helper
  [[maybe_unused]] const char* alsa_name(const snd_pcm_format_t format)
  { return snd_pcm_format_name(format); }

  [[maybe_unused]] const char* alsa_name(const snd_pcm_state_t state)
  { return snd_pcm_state_name (state); }

  class AlsaDevice {
  public:
    AlsaDevice(const std::string& name);
    ~AlsaDevice();
    void open();
    void close();
    AlsaDeviceConfig setup(const AlsaDeviceConfig& config);
    void prepare();
    void start();
    void write(const void* data, size_t bytes);
    void drop();
    void drain();
    AlsaDeviceCaps grope();
    snd_pcm_state_t state();
    microseconds delay();

  private:
    std::string name_;
    snd_pcm_t* pcm_;
    unsigned int rate_; // cache
  };

  AlsaDevice::AlsaDevice(const std::string& name)
    : name_ {name},
      pcm_ {nullptr}
  {}

  AlsaDevice::~AlsaDevice()
  {
    if (pcm_)
      try { close(); }
      catch(AlsaError& e) {}
  }

  void AlsaDevice::open()
  {
    if (pcm_) return;

    int error = snd_pcm_open(&pcm_, name_.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (error < 0)
      throw AlsaError {"failed to open device '" + name_ + "'", error};
  }

  void AlsaDevice::close()
  {
    if (!pcm_) return;

    int error = snd_pcm_close(pcm_);
    if (error < 0)
      throw AlsaError {"failed to close device", error};
    else
      pcm_ = nullptr;
  }

  AlsaDeviceConfig AlsaDevice::setup(const AlsaDeviceConfig& in_cfg)
  {
    assert(pcm_ != nullptr && "can not prepare a closed device");
    assert(state() == SND_PCM_STATE_OPEN);

    AlsaDeviceConfig out_cfg = in_cfg;

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    int error = snd_pcm_hw_params_any(pcm_, hw_params);
    if (error < 0)
      throw AlsaError {"failed to set up configuration space", error};

    error = snd_pcm_hw_params_set_access(pcm_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (error < 0)
      throw AlsaError {"failed to set the access type", error};

    error = snd_pcm_hw_params_set_format(pcm_, hw_params, in_cfg.format);
    if (error < 0)
      throw AlsaError {"failed to set the sample format", error};

    error = snd_pcm_hw_params_set_channels_near(pcm_, hw_params, &out_cfg.channels );
    if (error < 0)
      throw AlsaError {"failed to set the number of channels", error};

    error = snd_pcm_hw_params_set_rate_near(pcm_, hw_params, &out_cfg.rate, NULL);
    if (error < 0)
      throw AlsaError {"failed to set the sample rate", error};

    error = snd_pcm_hw_params_set_periods_near(pcm_, hw_params, &out_cfg.periods, NULL);
    if (error < 0)
      throw AlsaError {"failed to set the number of periods", error};

    error = snd_pcm_hw_params_set_period_size_near(pcm_, hw_params, &out_cfg.period_size, NULL);
    if (error < 0)
      throw AlsaError {"failed to set minimum period size", error};

    // enter state SND_PCM_STATE_PREPARED
    error = snd_pcm_hw_params(pcm_, hw_params);
    if (error < 0)
      throw AlsaError {"failed to install pcm hardware configuration", error};

    rate_ = out_cfg.rate;

    return out_cfg;
  }

  void AlsaDevice::prepare()
  {
    assert(pcm_ != nullptr && "can not prepare a closed device");
    assert(state() == SND_PCM_STATE_PREPARED || state() == SND_PCM_STATE_SETUP);

    int error = snd_pcm_prepare(pcm_);
    if (error < 0)
      throw AlsaError {"failed to prepare device", error};
  }

  void AlsaDevice::start()
  {
    assert(pcm_ != nullptr && "can not start a closed device");
    assert(state() == SND_PCM_STATE_PREPARED);

    int error = snd_pcm_start(pcm_);
    if (error < 0)
      throw AlsaError {"failed to start device", error};
  }

  void AlsaDevice::write(const void* data, size_t bytes)
  {
    assert(pcm_ != nullptr && "can not write to a closed device");

    snd_pcm_sframes_t frames = snd_pcm_bytes_to_frames(pcm_, bytes);
// #ifndef NDEBUG
//     std::cout << "AlsaBackend: write " << frames << " frames" << std::endl;
// #endif
    snd_pcm_sframes_t frames_written = snd_pcm_writei(pcm_, data, frames);
    if (frames_written < 0)
    {
      frames_written = snd_pcm_recover(pcm_, frames_written, 0);
    }
    if (frames_written < 0)
      throw AlsaError {"write failed (could not recover)", (int)frames_written};
    else if (frames_written > 0 && frames_written < (snd_pcm_sframes_t) frames)
    {
#ifndef NDEBUG
      std::cerr << "AlsaBackend: short write (expected " << frames
                << ", wrote " << frames_written << " frames)" << std::endl;
#endif
    }
  }

  void AlsaDevice::drop()
  {
    assert(pcm_ != nullptr && "can not stop (drop) a closed device");

    int error = snd_pcm_drop(pcm_);
    if (error < 0)
      throw AlsaError {"failed to stop (drop) device", error};
  }

  void AlsaDevice::drain()
  {
    assert(pcm_ != nullptr && "can not stop (drain) a closed device");

    int error = snd_pcm_drain(pcm_);
    if (error < 0)
      throw AlsaError {"failed to stop (drain) device", error};
  }

  AlsaDeviceCaps AlsaDevice::grope()
  {
    assert(pcm_ != nullptr && "can not grope a closed device");
    assert(state() == SND_PCM_STATE_OPEN);

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    int error = snd_pcm_hw_params_any(pcm_, hw_params);
    if (error < 0)
      throw AlsaError {"failed to set up configuration space", error};

    std::vector<snd_pcm_format_t> formats;

    std::vector<snd_pcm_format_t> test_formats = {
      SND_PCM_FORMAT_S16_LE
    };
    for (const auto& fmt : test_formats)
    {
      error = snd_pcm_hw_params_test_format(pcm_, hw_params, fmt);
      if (error < 0)
        throw AlsaError {"failed to test format ", error};

      formats.push_back(fmt);
    }

    unsigned int min_channels {0};
    unsigned int max_channels {0};

    error = snd_pcm_hw_params_get_channels_min( hw_params, &min_channels );
    if (error < 0)
      throw AlsaError {"failed to get minimum number of channels", error};

    snd_pcm_hw_params_get_channels_max( hw_params, &max_channels );
    if (error < 0)
      throw AlsaError {"failed to get maximum number of channels", error};

    // don't allow rate resampling when probing for the rate range
    snd_pcm_hw_params_set_rate_resample( pcm_, hw_params, 0 );

    unsigned int min_rate = 0;
    unsigned int max_rate = 0;

    error = snd_pcm_hw_params_get_rate_min (hw_params, &min_rate, NULL);
    if (error < 0)
      throw AlsaError {"failed to get minimum sample rate", error};

    error = snd_pcm_hw_params_get_rate_max (hw_params, &max_rate, NULL);
    if (error < 0)
      throw AlsaError {"failed to get maximum sample rate", error};

    unsigned int min_periods = 0;
    unsigned int max_periods = 0;

    error = snd_pcm_hw_params_get_periods_min (hw_params, &min_periods, NULL);
    if (error < 0)
      throw AlsaError {"failed to get minimum number of periods", error};

    error = snd_pcm_hw_params_get_periods_max (hw_params, &max_periods, NULL);
    if (error < 0)
      throw AlsaError {"failed to get maximum number of periods", error};

    snd_pcm_uframes_t min_period_size = 0;
    snd_pcm_uframes_t max_period_size = 0;

    error = snd_pcm_hw_params_get_period_size_min (hw_params, &min_period_size, NULL);
    if (error < 0)
      throw AlsaError {"failed to get minimum period size", error};

    error = snd_pcm_hw_params_get_period_size_max (hw_params, &max_period_size, NULL);
    if (error < 0)
      throw AlsaError {"failed to get maximum period size", error};

    snd_pcm_uframes_t min_buffer_size = 0;
    snd_pcm_uframes_t max_buffer_size = 0;

    error = snd_pcm_hw_params_get_buffer_size_min (hw_params, &min_buffer_size);
    if (error < 0)
      throw AlsaError {"failed to get minimum buffer size", error};

    error = snd_pcm_hw_params_get_buffer_size_max (hw_params, &max_buffer_size);
    if (error < 0)
      throw AlsaError {"failed to get maximum buffer size", error};

    AlsaDeviceCaps caps;

    caps.formats = formats;
    caps.min_channels = min_channels;
    caps.max_channels = max_channels;
    caps.min_rate = min_rate;
    caps.max_rate = max_rate;
    caps.min_periods = min_periods;
    caps.max_periods = max_periods;
    caps.min_period_size = min_period_size;
    caps.max_period_size = max_period_size;
    caps.min_buffer_size = min_buffer_size;
    caps.max_buffer_size = max_buffer_size;

    return caps;
  }

  snd_pcm_state_t AlsaDevice::state()
  {
    assert(pcm_ != nullptr && "attempt to get the state of a closed device");
    return snd_pcm_state(pcm_);
  }

  microseconds AlsaDevice::delay()
  {
    assert(pcm_ != nullptr && "attempt to obtain delay of a closed device");

    snd_pcm_sframes_t frames;
    int error = snd_pcm_delay (pcm_, &frames);
    if (error < 0)
      throw AlsaError {"failed to obtain delay", error};

    return microseconds((frames * std::micro::den) / rate_);
  }

  std::vector<AlsaDeviceDescription> scanAlsaDevices()
  {
    std::vector<AlsaDeviceDescription> devices;

    void **hints, **n;
    char *name, *descr, *io;

    int error = snd_device_name_hint(-1, "pcm", &hints);
    if (error < 0)
      throw AlsaError {"failed to scan alsa pcm devices", error};

    n = hints;

    while (*n != NULL)
    {
      name = snd_device_name_get_hint(*n, "NAME");
      descr = snd_device_name_get_hint(*n, "DESC");
      io = snd_device_name_get_hint(*n, "IOID");

      if (io == NULL || strcmp(io, "Output") == 0)
      {
        if (name != NULL)
        {
          if (descr != NULL)
            devices.push_back({name, descr});
          else
            devices.push_back({name, ""});
        }
      }

      if (name != NULL)
        free(name);
      if (descr != NULL)
        free(descr);
      if (io != NULL)
        free(io);
      n++;
    }

    snd_device_name_free_hint(hints);

    return devices;
  }

  bool validateAlsaDevice(const std::string& name,
                          bool grope_succeeded,
                          const AlsaDeviceCaps& caps)
  {
    static const std::vector<std::string> kPrefixes =
      {
        "null",
        //"samplerate",
        //"speexrate",
        //"pulse",
        //"speex",
        //"upmix",
        //"vdownmix",
        //"jack",
        "oss",
        //"surround"
      };

    if (!grope_succeeded)
    {
#ifndef NDEBUG
      std::cout << "AlsaBackend: ignoring device '" << name << "' ("
                << "could not determine device capabilities" << ")" << std::endl;
#endif
      return false;
    }

    // do some validity checks based on device capabilities
    if (caps.formats.empty())
    {
#ifndef NDEBUG
      std::cout << "AlsaBackend: ignoring device '" << name << "' ("
                << "could not find suitable sample format" << ")" << std::endl;
#endif
      return false;
    }

    if (caps.max_channels == 0 || caps.max_channels < caps.min_channels)
    {
#ifndef NDEBUG
      std::cout << "AlsaBackend: ignoring device '" << name << "' ("
                << "invalid channel configuration ["
                << caps.min_channels << ", " << caps.max_channels << "])" << std::endl;
#endif
      return false;
    }

    if (caps.max_rate == 0 || caps.max_rate < caps.min_rate)
    {
#ifndef NDEBUG
      std::cout << "AlsaBackend: ignoring device '" << name << "' ("
                << "invalid rate configuration ["
                << caps.min_channels << ", " << caps.max_channels << "])" << std::endl;
#endif
      return false;
    }

    return ! std::any_of ( kPrefixes.begin(), kPrefixes.end(),
                           [&name] (const auto& prefix) {
                             return name.compare(0, prefix.size(), prefix) == 0;
                           });
  }

  AlsaBackend::AlsaBackend()
    : state_ {BackendState::kConfig},
      cfg_ {kDefaultConfig},
      device_infos_ {},
      alsa_device_ {nullptr}
  {}

  AlsaBackend::~AlsaBackend() {}

  std::vector<DeviceInfo> AlsaBackend::devices()
  {
    scanDevices();
    return device_infos_;
  }

  void AlsaBackend::configure(const DeviceConfig& config)
  {
    if ( state_ != BackendState::kConfig )
      throw TransitionError(state_);

    cfg_ = config;
  }

  DeviceConfig AlsaBackend::configuration()
  { return cfg_; }

  DeviceConfig AlsaBackend::open()
  {
#ifndef NDEBUG
    std::cout << "AlsaBackend: open device '" << cfg_.name << "'" << std::endl;
#endif
    if ( state_ != BackendState::kConfig )
      throw TransitionError(state_);

    try {
      alsa_device_ = std::make_unique<AlsaDevice>(cfg_.name);
      alsa_device_->open();
    }
    catch(const AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }

    AlsaDeviceConfig alsa_in_cfg;
    alsa_in_cfg.format = SND_PCM_FORMAT_S16_LE;
    alsa_in_cfg.rate = cfg_.spec.rate;
    alsa_in_cfg.channels = cfg_.spec.channels;

    // TODO: Do not hardcode this!
    //       The preferred buffer configuration should depend on the client side
    //       configuration (audio::DeviceConfig) and might anticipate a working
    //       setup by respecting the actual device capabilities (audio::AlsaDeviceCaps).
    alsa_in_cfg.periods = 4;
    alsa_in_cfg.period_size = 1024;

#ifndef NDEBUG
    std::cout << "AlsaBackend: pre config: " << alsa_in_cfg << std::endl;
#endif

    AlsaDeviceConfig alsa_out_cfg = alsa_in_cfg;
    try {
      alsa_out_cfg = alsa_device_->setup(alsa_in_cfg);
    }
    catch(const AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }

#ifndef NDEBUG
    std::cout << "AlsaBackend: act config: " << alsa_out_cfg << std::endl;
#endif

    state_ = BackendState::kOpen;

    DeviceConfig actual_cfg;
    actual_cfg.name = cfg_.name;
    actual_cfg.spec.format = SampleFormat::S16LE;
    actual_cfg.spec.rate = alsa_out_cfg.rate;
    actual_cfg.spec.channels = alsa_out_cfg.channels;

    return actual_cfg;
  }

  void AlsaBackend::close()
  {
#ifndef NDEBUG
    std::cout << "AlsaBackend: close device '" << cfg_.name << "'" << std::endl;
#endif
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    alsa_device_ = nullptr;
    state_ = BackendState::kConfig;
  }

  void AlsaBackend::start()
  {
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    assert(alsa_device_ != nullptr);

#ifndef NDEBUG
      std::cerr << "AlsaBackend: start device" << std::endl;
#endif
    try {
      alsa_device_->prepare();
      alsa_device_->start();
    }
    catch(const AlsaError& e) {
#ifndef NDEBUG
      std::cerr << "AlsaBackend: could not start device (continue anyway)" << std::endl;
#endif
    }

    state_ = BackendState::kRunning;
  }

  void AlsaBackend::stop()
  {
#ifndef NDEBUG
      std::cout << "AlsaBackend: stop device " << std::endl;
#endif
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    assert(alsa_device_ != nullptr);

    try {
      alsa_device_->drain();
    }
    catch(AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }

    state_ = BackendState::kOpen;
  }

  void AlsaBackend::write(const void* data, size_t bytes)
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    try {
      alsa_device_->write(data, bytes);
    }
    catch(AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }
  }

  void AlsaBackend::flush()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    assert(alsa_device_ != nullptr);

    try {
      alsa_device_->drop();
    }
    catch(AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }
  }

  void AlsaBackend::drain()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    assert(alsa_device_ != nullptr);

    try {
      alsa_device_->drain();
    }
    catch(AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }
  }

  microseconds AlsaBackend::latency()
  {
    assert(alsa_device_ != nullptr);
    microseconds latency = 0us;
    try {
      latency = alsa_device_->delay();
    }
    catch(...) {
#ifndef NDEBUG
      std::cerr << "AlsaBackend: couldn't get latency (continue using "
                << latency.count() << "us)" << std::endl;
#endif
    };
    return latency;
  }

  BackendState AlsaBackend::state() const
  {
    return state_;
  }

  void AlsaBackend::scanDevices()
  {
#ifndef NDEBUG
    std::cout << "AlsaBackend: scan devices" << std::endl;
#endif
    std::vector<AlsaDeviceDescription> device_descriptions;
    try {
      device_descriptions = scanAlsaDevices();
    }
    catch(AlsaError& e) {
      throw makeAlsaBackendError(state_, e);
    }

    std::vector<DeviceInfo> device_infos;
    device_infos.reserve( device_descriptions.size() );

    for (auto& device_descr : device_descriptions)
    {
      try {
        AlsaDevice alsa_device (device_descr.name);

        alsa_device.open();

        AlsaDeviceCaps device_caps;
        bool grope_succeeded = true;

        try {
          device_caps = alsa_device.grope();
        }
        catch (...)
        { grope_succeeded = false; }

        if (!validateAlsaDevice(device_descr.name, grope_succeeded, device_caps))
          continue;

        audio::DeviceInfo device_info;

        device_info.name = device_descr.name;
        device_info.descr = device_descr.descr;
        device_info.min_channels = device_caps.min_channels;
        device_info.max_channels = device_caps.max_channels;

        device_info.channels = std::max(
          std::min(kDefaultChannels, device_caps.max_channels),
          device_caps.min_channels);

        device_info.min_rate = device_caps.min_rate;
        device_info.max_rate = device_caps.max_rate;

        device_info.rate = std::max(
          std::min(kDefaultRate, device_caps.max_rate),
          device_caps.min_rate);

        device_infos.push_back(device_info);

        alsa_device.close();
      }
      catch(...) {} // ignore error and try next
    }
    std::swap(device_infos_,device_infos);
#ifndef NDEBUG
    std::cout << "AlsaBackend: " << device_descriptions.size() << " devices found ("
              << device_infos_.size() << " usable)" << std::endl;
#endif
  }

}//namespace audio
