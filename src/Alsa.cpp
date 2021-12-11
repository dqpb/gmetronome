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
#include <map>
#include <algorithm>
#include <thread>
#include <iostream>

namespace audio {

  namespace {

    class AlsaError : public BackendError {
    public:
      AlsaError(BackendState state, const char* what = "", int error = 0)
        : BackendError(settings::kAudioBackendAlsa, state, what),
          error_(error)
      {}
      AlsaError(BackendState state, int error, const char* what = "")
        : AlsaError(state, what, error)
      {}
    private:
      int error_;
    };

    class TransitionError : public AlsaError {
    public:
      TransitionError(BackendState state)
        : AlsaError(state, "invalid state transition")
      {}
    };

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

    struct AlsaDeviceInfo
    {
      std::string name;
      std::string descr;
      int  min_channels {0};
      int  max_channels {0};
      int  channels {0};
      double min_rate {0};
      double max_rate {0};
      double rate {0};
      int min_periods {0};
      int max_periods {0};
      unsigned long min_period_size {0};
      unsigned long max_period_size {0};
      unsigned long min_buffer_size {0};
      unsigned long max_buffer_size {0};
    };

    [[maybe_unused]] std::ostream& operator<<(std::ostream& os, const AlsaDeviceInfo& dev)
    {
      std::string headline = "Device [" + dev.name + "]";

      os << headline << std::endl << std::string(headline.size(),'=') << std::endl;

      os << "  Name           : " << dev.name << std::endl
         << "  Description    : " << dev.descr << std::endl
         << "  Channels Range : ["
         << dev.min_channels << "," << dev.max_channels << "]" << std::endl
         << "  Channels       : " << dev.channels << std::endl
         << "  Rate Range     : ["
         << dev.min_rate << "," << dev.max_rate << "]" << std::endl
         << "  Rate           : " << dev.rate << std::endl
         << "  Periods        : ["
         << dev.min_periods << "," << dev.max_periods << "]" << std::endl
         << "  Period Size    : ["
         << dev.min_period_size << "," << dev.max_period_size << "]" << std::endl
         << "  Buffer Size    : ["
         << dev.min_buffer_size << "," << dev.max_buffer_size << "]" << std::endl;

      return  os;
    }

    bool ignoreAlsaDevice( const std::string& name )
    {
      static const std::vector<std::string> kINames =
      {
        "null",
        "samplerate",
        "speexrate",
        "pulse",
        //"speex",
        //"upmix",
        //"vdownmix",
        "jack",
        "oss",
        //"surround"
      };

      return std::any_of ( kINames.begin(), kINames.end(),
                           [&name] (const auto& iname) {
                             return name.compare(0, iname.size(), iname) == 0;
                           });
    }

    bool gropeAlsaDevice(AlsaDeviceInfo& device)
    {
      const char* pcm_name = device.name.c_str();
      snd_pcm_t* pcm_handle = NULL;
      bool success = true;

      int err = snd_pcm_open( &pcm_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0 );
      if (err < 0)
      {
        std::cerr << "AlsaBackend: could not open output device "
                  << "[" << device.name << "]: " << snd_strerror(err) << std::endl;
        return false;
      }

      snd_pcm_hw_params_t *hwParams;
      snd_pcm_hw_params_alloca(&hwParams);

      if (snd_pcm_hw_params_any(pcm_handle, hwParams) < 0) {
        std::cerr << "AlsaBackend: can not configure PCM device "
                  << "[" << device.name << "]" << std::endl;
        snd_pcm_close( pcm_handle );
        return false;
      }

      unsigned int minChans;
      unsigned int maxChans;

      snd_pcm_hw_params_get_channels_min( hwParams, &minChans );
      snd_pcm_hw_params_get_channels_max( hwParams, &maxChans );

      // limit number of device channels
      if( maxChans > 128 )
        maxChans = 128;

      if ( maxChans == 0 )
      {
        std::cerr << "AlsaBackend: invalid max channels for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      unsigned int channels = kDefaultChannels;
      err = snd_pcm_hw_params_set_channels_near( pcm_handle, hwParams, &channels );
      if (err < 0)
      {
        std::cerr << "AlsaBackend: failed to set channels for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      // don't allow rate resampling when probing for the default rate
      snd_pcm_hw_params_set_rate_resample( pcm_handle, hwParams, 0 );

      unsigned int minRate = 0;
      unsigned int maxRate = 0;

      err = snd_pcm_hw_params_get_rate_min (hwParams, &minRate, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get min sample rate for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_rate_max (hwParams, &maxRate, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get max sample rate for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      unsigned int testRate = kDefaultRate;
      err = snd_pcm_hw_params_set_rate_near( pcm_handle, hwParams, &testRate, NULL );
      if (err < 0)
      {
        std::cerr << "AlsaBackend: failed to set sample rate for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      double defaultSR;
      unsigned int num, den = 1;
      if (snd_pcm_hw_params_get_rate_numden( hwParams, &num, &den ) < 0)
      {
        std::cerr << "AlsaBackend: failed to get sample rate for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
        defaultSR = 0;
      }
      else
      {
        defaultSR = (double) num / den;
      }

      if (defaultSR > maxRate || defaultSR < minRate) {
        std::cerr << "AlsaBackend: invalid default sample rate for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      unsigned int minPds = 0;
      unsigned int maxPds = 0;
      err = snd_pcm_hw_params_get_periods_min ( hwParams, &minPds, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get minimum number of periods for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_periods_max ( hwParams, &maxPds, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get maximum number of periods for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      snd_pcm_uframes_t  minPdSize = 0;
      snd_pcm_uframes_t  maxPdSize = 0;
      err = snd_pcm_hw_params_get_period_size_min ( hwParams, &minPdSize, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get minimum period size for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_period_size_max ( hwParams, &maxPdSize, NULL);
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get maximum period size for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      snd_pcm_uframes_t  minBufSize = 0;
      snd_pcm_uframes_t  maxBufSize = 0;
      err = snd_pcm_hw_params_get_buffer_size_min ( hwParams, &minBufSize );
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get minimum buffer size for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_buffer_size_max ( hwParams, &maxBufSize );
      if (err < 0) {
        std::cerr << "AlsaBackend: failed to get maximum buffer size for device "
                  << "[" << device.name << "]" << std::endl;
        success = false;
      }

      snd_pcm_close( pcm_handle );

      if (success)
      {
        device.min_channels = minChans;
        device.max_channels = maxChans;
        device.channels = channels;
        device.min_rate = minRate;
        device.max_rate = maxRate;
        device.rate = defaultSR;
        device.min_periods = minPds;
        device.max_periods = maxPds;
        device.min_period_size = minPdSize;
        device.max_period_size = maxPdSize;
        device.min_buffer_size = minBufSize;
        device.max_buffer_size = maxBufSize;
      }

      return success;
    }
    
    std::vector<AlsaDeviceInfo> scanAlsaDevices()
    {
      std::vector<AlsaDeviceInfo> devices;
      
      void **hints, **n;
      char *name, *descr, *io;
      
      if (snd_device_name_hint(-1, "pcm", &hints) < 0)
        throw std::runtime_error("failed to scan alsa pcm devices [snd_device_name_hint]");

      n = hints;

      while (*n != NULL)
      {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");

        if (io == NULL || strcmp(io, "Output") == 0)
        {
          if (name != NULL && !ignoreAlsaDevice(name) )
          {
            AlsaDeviceInfo device;

            device.name = name;

            if (descr != NULL)
              device.descr = descr;

            devices.push_back(device);
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

    const microseconds kRequiredLatency = 80ms;

  }//unnamed namespace


  AlsaBackend::AlsaBackend()
    : state_(BackendState::kConfig),
      cfg_(kDefaultConfig),
      devs_({}),
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
    scanDevices();
    return devs_;
  }

  void AlsaBackend::configure(const DeviceConfig& config)
  {
    std::cout << __PRETTY_FUNCTION__ << " : " << config.name << std::endl;
    cfg_ = config;
  }

  DeviceConfig AlsaBackend::configuration()
  { return cfg_; }

  DeviceConfig AlsaBackend::open()
  {
    std::cout << __PRETTY_FUNCTION__ << " : " << cfg_.name << std::endl;

    std::cout << "Open-config [" << cfg_.name << "]" << std::endl;
    std::cout << " format   : " << snd_pcm_format_name(convertSampleFormatToAlsa(cfg_.spec.format)) << std::endl;
    std::cout << " rate     : " << cfg_.spec.rate << std::endl;
    std::cout << " channels : " << cfg_.spec.channels << std::endl;

    if ( state_ != BackendState::kConfig )
      throw TransitionError(state_);

    const char* pcm_name = ( cfg_.name.empty() ? "default" : cfg_.name.c_str() );

    int error = 0;
    
    if (hdl_ == nullptr)
    {
      error = snd_pcm_open(&hdl_, pcm_name, SND_PCM_STREAM_PLAYBACK, 0);
      for (int retry = 0; error == -EBUSY && retry < 2 ; ++retry)
      {
        std::this_thread::sleep_for(500ms);
        error = snd_pcm_open(&hdl_, pcm_name, SND_PCM_STREAM_PLAYBACK, 0);
      }
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_open]");
    }

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_format_t format = convertSampleFormatToAlsa(cfg_.spec.format);
    snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
    unsigned int channels = cfg_.spec.channels;
    unsigned int rate = cfg_.spec.rate;
    unsigned int buffer_time = kRequiredLatency.count(); //us

    try {
      error = snd_pcm_hw_params_any(hdl_, hw_params);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_any]");

      error = snd_pcm_hw_params_set_format(hdl_, hw_params, format);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params_set_format]");

      error = snd_pcm_hw_params_set_access(hdl_, hw_params, access);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params_set_access]");

      error = snd_pcm_hw_params_set_channels_near(hdl_, hw_params, &channels);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params_set_channels_near]");

      error = snd_pcm_hw_params_set_rate_near(hdl_, hw_params, &rate, NULL);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params_set_rate_near]");

      error = snd_pcm_hw_params_set_buffer_time_near(hdl_, hw_params, &buffer_time, NULL);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params_set_buffer_time_near]");

      error = snd_pcm_hw_params(hdl_, hw_params);
      if (error < 0)
        throw AlsaError(state_, error, "open() [snd_pcm_hw_params]");
    }
    catch(...)
    {
      if ( snd_pcm_close(hdl_) >= 0 )
        hdl_ = nullptr;

      throw;
    }

    state_ = BackendState::kOpen;

    DeviceConfig actual_cfg;
    actual_cfg.name = pcm_name;
    actual_cfg.spec.format = cfg_.spec.format;
    actual_cfg.spec.rate = rate;
    actual_cfg.spec.channels = channels;

    std::cout << "Actual config [" << actual_cfg.name << "]" << std::endl;
    std::cout << " format   : " << snd_pcm_format_name(format) << std::endl;
    std::cout << " rate     : " << actual_cfg.spec.rate << std::endl;
    std::cout << " channels : " << actual_cfg.spec.channels << std::endl;

    return actual_cfg;
  }

  void AlsaBackend::close()
  {
    std::cout << __PRETTY_FUNCTION__ << " : " << cfg_.name << std::endl;
    
    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    int error;

    error = snd_pcm_close(hdl_);
    if (error < 0)
      throw AlsaError(state_, error, "close() [snd_pcm_close]");

    hdl_ = nullptr;
    state_ = BackendState::kConfig;
  }

  void AlsaBackend::start()
  {
    std::cout << __PRETTY_FUNCTION__ << " : " << cfg_.name << std::endl;

    if ( state_ != BackendState::kOpen )
      throw TransitionError(state_);

    int error;

    error = snd_pcm_prepare(hdl_);
    if (error < 0)
      throw AlsaError(state_, error, "start() [snd_pcm_prepare]");

    state_ = BackendState::kRunning;
  }

  void AlsaBackend::stop()
  {
    std::cout << __PRETTY_FUNCTION__ << " : " << cfg_.name << std::endl;

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

    snd_pcm_sframes_t n_data_frames = bytes / frameSize(cfg_.spec);
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
      std::cerr << "AlsaBackend: short write (expected " << n_data_frames
                << ", wrote " << frames_written << " frames)"
                << std::endl;
    }
  }

  void AlsaBackend::flush()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    int error = snd_pcm_drop(hdl_);
    if (error < 0)
      throw AlsaError(state_, error, "flush() [snd_pcm_drop]");
  }

  void AlsaBackend::drain()
  {
    if ( state_ != BackendState::kRunning )
      throw TransitionError(state_);

    int error = snd_pcm_drain(hdl_);
    if (error < 0)
      throw AlsaError(state_, error, "drain() [snd_pcm_drain]");
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
      return framesToUsecs(delayp, cfg_.spec);
  }

  BackendState AlsaBackend::state() const
  {
    return state_;
  }

  void AlsaBackend::scanDevices()
  {
    std::vector<AlsaDeviceInfo> alsa_device_list;
    
    try {
      alsa_device_list = scanAlsaDevices();
    }
    catch(std::exception& e) {
      throw AlsaError(state_, e.what());
    }
    
    std::vector<DeviceInfo> device_list;
    device_list.reserve( alsa_device_list.size() );

    for (auto& alsa_device : alsa_device_list)
    {      
      if ( gropeAlsaDevice(alsa_device) )
      {
        audio::DeviceInfo device_info;

        device_info.name = alsa_device.name;
        device_info.descr = alsa_device.descr;
        device_info.min_channels = alsa_device.min_channels;
        device_info.max_channels = alsa_device.max_channels;
        device_info.channels = alsa_device.channels;
        device_info.min_rate = alsa_device.min_rate;
        device_info.max_rate = alsa_device.max_rate;
        device_info.rate = alsa_device.rate;

        device_list.push_back(device_info);
      }
    }
    std::swap(devs_,device_list);
  }

}//namespace audio
