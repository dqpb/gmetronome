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

#ifndef GMetronome_Alsa_h
#define GMetronome_Alsa_h

#include "AudioBackend.h"
#include <alsa/asoundlib.h>
#include <memory>

namespace audio {

  /**
   * Alsa Backend
   */
  class AlsaBackend : public Backend
  {
  public:
    AlsaBackend();
    AlsaBackend(const AlsaBackend&) = delete;
    AlsaBackend(AlsaBackend&&) noexcept;
    ~AlsaBackend();

    AlsaBackend& operator=(const AlsaBackend&) = delete;
    AlsaBackend& operator=(AlsaBackend&&) noexcept;

    std::vector<DeviceInfo> devices() override;
    void configure(const DeviceConfig& config) override;
    DeviceConfig configuration() override;
    DeviceConfig open() override;
    void close() override;
    void start() override;
    void stop() override;
    void write(const void* data, size_t bytes) override;
    void flush() override;
    void drain() override;
    microseconds latency() override;
    BackendState state() const override;

  private:

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

    struct AlsaDeviceConfig
    {
      snd_pcm_format_t format;
      unsigned int channels;
      unsigned int rate;
      snd_pcm_uframes_t period_size;
      snd_pcm_uframes_t buffer_size;
    };

    class AlsaDevice {
    public:
      explicit AlsaDevice(const std::string& name);
      AlsaDevice(const AlsaDevice& device) = delete;
      AlsaDevice(AlsaDevice&& device) noexcept;
      ~AlsaDevice();

      AlsaDevice& operator=(const AlsaDevice& device) = delete;
      AlsaDevice& operator=(AlsaDevice&& device) noexcept;

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

      static std::vector<AlsaDeviceDescription> getAvailableDevices();

    private:
      std::string name_;
      snd_pcm_t* pcm_;
      unsigned int rate_; // cache
    };

    BackendState state_;
    audio::DeviceConfig cfg_;
    std::vector<DeviceInfo> device_infos_;
    std::unique_ptr<AlsaDevice> alsa_device_;

    bool validateAlsaDevice(const std::string& name,
                            bool open_succeeded,
                            bool grope_succeeded,
                            const AlsaDeviceCaps& caps);

    void scanAlsaDevices();

    // debug helper
    friend std::ostream& operator<<(std::ostream&, const AlsaDeviceCaps&);
    friend std::ostream& operator<<(std::ostream&, const AlsaDeviceConfig&);
  };

}//namespace audio
#endif//GMetronome_Alsa_h
