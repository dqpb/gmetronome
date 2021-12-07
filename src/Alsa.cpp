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

    const std::string& alsaFormatString(snd_pcm_format_t format)
    {
      static const std::map<snd_pcm_format_t, std::string> format_map =
      {
        {SND_PCM_FORMAT_UNKNOWN, "SND_PCM_FORMAT_UNKNOWN"},
        {SND_PCM_FORMAT_S8, "SND_PCM_FORMAT_S8"},
        {SND_PCM_FORMAT_U8, "SND_PCM_FORMAT_U8"},
        {SND_PCM_FORMAT_S16_LE, "SND_PCM_FORMAT_S16_LE"},
        {SND_PCM_FORMAT_S16_BE, "SND_PCM_FORMAT_S16_BE"},
        {SND_PCM_FORMAT_U16_LE, "SND_PCM_FORMAT_U16_LE"},
        {SND_PCM_FORMAT_U16_BE, "SND_PCM_FORMAT_U16_BE"},
        {SND_PCM_FORMAT_S24_LE, "SND_PCM_FORMAT_S24_LE"},
        {SND_PCM_FORMAT_S24_BE, "SND_PCM_FORMAT_S24_BE"},
        {SND_PCM_FORMAT_U24_LE, "SND_PCM_FORMAT_U24_LE"},
        {SND_PCM_FORMAT_U24_BE, "SND_PCM_FORMAT_U24_BE"},
        {SND_PCM_FORMAT_S32_LE, "SND_PCM_FORMAT_S32_LE"},
        {SND_PCM_FORMAT_S32_BE, "SND_PCM_FORMAT_S32_BE"},
        {SND_PCM_FORMAT_U32_LE, "SND_PCM_FORMAT_U32_LE"},
        {SND_PCM_FORMAT_U32_BE, "SND_PCM_FORMAT_U32_BE"},
        {SND_PCM_FORMAT_FLOAT_LE, "SND_PCM_FORMAT_FLOAT_LE"},
        {SND_PCM_FORMAT_FLOAT_BE, "SND_PCM_FORMAT_FLOAT_BE"},
        {SND_PCM_FORMAT_FLOAT64_LE, "SND_PCM_FORMAT_FLOAT64_LE"},
        {SND_PCM_FORMAT_FLOAT64_BE, "SND_PCM_FORMAT_FLOAT64_BE"},
        {SND_PCM_FORMAT_IEC958_SUBFRAME_LE, "SND_PCM_FORMAT_IEC958_SUBFRAME_LE"},
        {SND_PCM_FORMAT_IEC958_SUBFRAME_BE, "SND_PCM_FORMAT_IEC958_SUBFRAME_BE"},
        {SND_PCM_FORMAT_MU_LAW, "SND_PCM_FORMAT_MU_LAW"},
        {SND_PCM_FORMAT_A_LAW, "SND_PCM_FORMAT_A_LAW"},
        {SND_PCM_FORMAT_IMA_ADPCM, "SND_PCM_FORMAT_IMA_ADPCM"},
        {SND_PCM_FORMAT_MPEG, "SND_PCM_FORMAT_MPEG"},
        {SND_PCM_FORMAT_GSM, "SND_PCM_FORMAT_GSM"},
        {SND_PCM_FORMAT_SPECIAL, "SND_PCM_FORMAT_SPECIAL"},
        {SND_PCM_FORMAT_S24_3LE, "SND_PCM_FORMAT_S24_3LE"},
        {SND_PCM_FORMAT_S24_3BE, "SND_PCM_FORMAT_S24_3BE"},
        {SND_PCM_FORMAT_U24_3LE, "SND_PCM_FORMAT_U24_3LE"},
        {SND_PCM_FORMAT_U24_3BE, "SND_PCM_FORMAT_U24_3BE"},
        {SND_PCM_FORMAT_S20_3LE, "SND_PCM_FORMAT_S20_3LE"},
        {SND_PCM_FORMAT_S20_3BE, "SND_PCM_FORMAT_S20_3BE"},
        {SND_PCM_FORMAT_U20_3LE, "SND_PCM_FORMAT_U20_3LE"},
        {SND_PCM_FORMAT_U20_3BE, "SND_PCM_FORMAT_U20_3BE"},
        {SND_PCM_FORMAT_S18_3LE, "SND_PCM_FORMAT_S18_3LE"},
        {SND_PCM_FORMAT_S18_3BE, "SND_PCM_FORMAT_S18_3BE"},
        {SND_PCM_FORMAT_U18_3LE, "SND_PCM_FORMAT_U18_3LE"},
        {SND_PCM_FORMAT_U18_3BE, "SND_PCM_FORMAT_U18_3BE"},
        {SND_PCM_FORMAT_G723_24, "SND_PCM_FORMAT_G723_24"},
        {SND_PCM_FORMAT_G723_24_1B, "SND_PCM_FORMAT_G723_24_1B"},
        {SND_PCM_FORMAT_G723_40, "SND_PCM_FORMAT_G723_40"},
        {SND_PCM_FORMAT_G723_40_1B, "SND_PCM_FORMAT_G723_40_1B"},
        {SND_PCM_FORMAT_DSD_U8, "SND_PCM_FORMAT_DSD_U8"},
        {SND_PCM_FORMAT_DSD_U16_LE, "SND_PCM_FORMAT_DSD_U16_LE"},
        {SND_PCM_FORMAT_LAST, "SND_PCM_FORMAT_LAST"},
        {SND_PCM_FORMAT_S16, "SND_PCM_FORMAT_S16"},
        {SND_PCM_FORMAT_U16, "SND_PCM_FORMAT_U16"},
        {SND_PCM_FORMAT_S24, "SND_PCM_FORMAT_S24"},
        {SND_PCM_FORMAT_U24, "SND_PCM_FORMAT_U24"},
        {SND_PCM_FORMAT_S32, "SND_PCM_FORMAT_S32"},
        {SND_PCM_FORMAT_U32, "SND_PCM_FORMAT_U32"},
        {SND_PCM_FORMAT_FLOAT, "SND_PCM_FORMAT_FLOAT"},
        {SND_PCM_FORMAT_FLOAT64, "SND_PCM_FORMAT_FLOAT64"},
        {SND_PCM_FORMAT_IEC958_SUBFRAME, "SND_PCM_FORMAT_IEC958_SUBFRAME"}
      };

      auto it = format_map.find(format);

      if (it == format_map.end())
        return format_map.at(SND_PCM_FORMAT_UNKNOWN);
      else
        return it->second;
    }

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

    std::ostream& operator<<(std::ostream& os, const AlsaDeviceInfo& dev)
    {
      std::string headline = "Device [" + dev.name + "]";

      os << headline << std::endl << std::string(headline.size(),'=') << std::endl;

      os << "  Name         : " << dev.name << std::endl
         << "  Description  : " << dev.descr << std::endl
         << "  Channels     : ["
         << dev.min_channels << "," << dev.max_channels << "]" << std::endl
         << "  Default Ch   : " << dev.channels << std::endl
         << "  Rate         : ["
         << dev.min_rate << "," << dev.max_rate << "]" << std::endl
         << "  Default Rate : " << dev.rate << std::endl
         << "  Periods      : ["
         << dev.min_periods << "," << dev.max_periods << "]" << std::endl
         << "  Period Size  : ["
         << dev.min_period_size << "," << dev.max_period_size << "]" << std::endl
         << "  Buffer Size  : ["
         << dev.min_buffer_size << "," << dev.max_buffer_size << "]" << std::endl;

      return  os;
    }

    /*
    const AlsaDeviceInfo& predefinedDeviceInfo( const std::string& name )
    {
      static const std::vector<AlsaDeviceInfo> predefined_names =
      {
        { "center_lfe", "", -1, 0, 1, 0 },
        // { "default", "", -1, 0, 1, 1 },
        { "dmix", "", -1, 0, 1, 0 },
        // { "dpl", "", -1, 0, 1, 0 },
        // { "dsnoop", "", -1, 0, 0, 1 },
        { "front", "", -1, 0, 1, 0 },
        { "iec958", "", -1, 0, 1, 0 },
        // { "modem", "", -1, 0, 1, 0 },
        { "rear", "", -1, 0, 1, 0 },
        { "side", "", -1, 0, 1, 0 },
        // { "spdif", "", -1, 0, 0, 0 },
        { "surround40", "", -1, 0, 1, 0 },
        { "surround41", "", -1, 0, 1, 0 },
        { "surround50", "", -1, 0, 1, 0 },
        { "surround51", "", -1, 0, 1, 0 },
        { "surround71", "", -1, 0, 1, 0 },

        { "AndroidPlayback_Earpiece_normal",         "", -1, 0, 1, 0 },
        { "AndroidPlayback_Speaker_normal",          "", -1, 0, 1, 0 },
        { "AndroidPlayback_Bluetooth_normal",        "", -1, 0, 1, 0 },
        { "AndroidPlayback_Headset_normal",          "", -1, 0, 1, 0 },
        { "AndroidPlayback_Speaker_Headset_normal",  "", -1, 0, 1, 0 },
        { "AndroidPlayback_Bluetooth-A2DP_normal",   "", -1, 0, 1, 0 },
        { "AndroidPlayback_ExtraDockSpeaker_normal", "", -1, 0, 1, 0 },
        { "AndroidPlayback_TvOut_normal",            "", -1, 0, 1, 0 },

        { "AndroidRecord_Microphone",                "", -1, 0, 0, 1 },
        { "AndroidRecord_Earpiece_normal",           "", -1, 0, 0, 1 },
        { "AndroidRecord_Speaker_normal",            "", -1, 0, 0, 1 },
        { "AndroidRecord_Headset_normal",            "", -1, 0, 0, 1 },
        { "AndroidRecord_Bluetooth_normal",          "", -1, 0, 0, 1 },
        { "AndroidRecord_Speaker_Headset_normal",    "", -1, 0, 0, 1 }
      };

      for (const auto& device : predefined_names)
      {
        if (device.name == name)
          return device;
      }
      return ;
    }
    */

    bool ignoreAlsaDevice( const std::string& name )
    {
      static const std::vector<std::string> kINames =
      {
        "null",
        "samplerate",
        "speexrate",
        "pulse",
        "speex",
        "upmix",
        "vdownmix",
        "jack",
        "oss",
        "surround"
      };

      return std::any_of ( kINames.begin(), kINames.end(),
                           [&name] (const auto& iname) {
                             return name.compare(0, iname.size(), iname) == 0;
                           });
    }
    

    /*
    std::string SkipCardDetailsInName( char *infoSkipName, const char *cardRefName )
    {
      char *lastSpacePosn = infoSkipName;

      while( *cardRefName )
        {
          while( *infoSkipName && *cardRefName && *infoSkipName == *cardRefName)
            {
              infoSkipName++;
              cardRefName++;
              if( *infoSkipName == ' ' || *infoSkipName == '\0' )
                lastSpacePosn = infoSkipName;
            }
          infoSkipName = lastSpacePosn;

          while( *cardRefName && ( *cardRefName++ != ' ' ));
        }
      if( *infoSkipName == '\0' )
        return "-"; 

      while( *lastSpacePosn && *lastSpacePosn == ' ' )
        lastSpacePosn++;

      if(( *lastSpacePosn == '-' || *lastSpacePosn == ':' ) && *(lastSpacePosn + 1) == ' ' )
        lastSpacePosn += 2;

      return lastSpacePosn;
    }
    */
    
    bool gropeAlsaDevice(AlsaDeviceInfo& device)
    {
      snd_pcm_t* pcm_handle = NULL;
      
      if (snd_pcm_open( &pcm_handle, device.name.c_str(),
                        SND_PCM_STREAM_PLAYBACK, 0 ) < 0)
      {
        std::cerr << "alsa: couldn't open output device [" << device.name << "]" << std::endl;
        return false;
      }
      
      snd_pcm_hw_params_t *hwParams;
      snd_pcm_hw_params_alloca(&hwParams);

      if (snd_pcm_hw_params_any(pcm_handle, hwParams) < 0) {
        std::cerr << "alsa: can not configure PCM device " << device.name << std::endl;
        snd_pcm_close( pcm_handle );
        return false;
      }

      const char* pcm_name = snd_pcm_name(pcm_handle);
      bool success = true;
      int err;
      
      unsigned int minChans;
      unsigned int maxChans;
            
      snd_pcm_hw_params_get_channels_min( hwParams, &minChans );
      snd_pcm_hw_params_get_channels_max( hwParams, &maxChans );
      
      // limit number of device channels
      if( maxChans > 128 )
        maxChans = 128;
      
      if ( maxChans == 0 )
      {
        std::cerr << "alsa: invalid max channels for device [" << device.name << "]" << std::endl;
        success = false;
      }

      unsigned int channels = kDefaultChannels;
      err = snd_pcm_hw_params_set_channels_near( pcm_handle, hwParams, &channels );
      if (err < 0)
      {
        std::cerr << "alsa: couldn't set channels for device [" << device.name << "]" << std::endl;
        success = false;
      }
      
      // don't allow rate resampling when probing for the default rate 
      snd_pcm_hw_params_set_rate_resample( pcm_handle, hwParams, 0 );
      
      unsigned int minRate = 0;
      unsigned int maxRate = 0;
      
      err = snd_pcm_hw_params_get_rate_min (hwParams, &minRate, NULL);
      if (err < 0) {
        std::cerr << "alsa: couldn't get min sample rate for device [" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_rate_max (hwParams, &maxRate, NULL);
      if (err < 0) {
        std::cerr << "alsa: couldn't get max sample rate for device [" << device.name << "]" << std::endl;
        success = false;
      }
      
      unsigned int testRate = kDefaultRate; 
      err = snd_pcm_hw_params_set_rate_near( pcm_handle, hwParams, &testRate, NULL );
      if (err < 0)
      {
        std::cerr << "alsa: couldn't set sample rate for device [" << device.name << "]" << std::endl;
        success = false;
      }
      
      double defaultSR;
      unsigned int num, den = 1;
      if (snd_pcm_hw_params_get_rate_numden( hwParams, &num, &den ) < 0)
      {
        std::cerr << "alsa: couldn't get sample rate for device [" << device.name << "]" << std::endl;
        success = false;
        defaultSR = 0;
      }
      else
      {
        defaultSR = (double) num / den;
      }
      
      if (defaultSR > maxRate || defaultSR < minRate) {
        std::cerr << "alsa: invalid default sample rate for device [" << device.name << "]" << std::endl;
        success = false;
      }
      
      unsigned int minPds = 0;
      unsigned int maxPds = 0;
      err = snd_pcm_hw_params_get_periods_min ( hwParams, &minPds, NULL); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get minimum number of periods for device [" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_periods_max ( hwParams, &maxPds, NULL); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get maximum number of periods for device [" << device.name << "]" << std::endl;
        success = false;
      }

      snd_pcm_uframes_t  minPdSize = 0;
      snd_pcm_uframes_t  maxPdSize = 0;
      err = snd_pcm_hw_params_get_period_size_min ( hwParams, &minPdSize, NULL); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get minimum period size for device [" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_period_size_max ( hwParams, &maxPdSize, NULL); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get maximum period size for device [" << device.name << "]" << std::endl;
        success = false;
      }

      snd_pcm_uframes_t  minBufSize = 0;
      snd_pcm_uframes_t  maxBufSize = 0;
      err = snd_pcm_hw_params_get_buffer_size_min ( hwParams, &minBufSize ); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get minimum buffer size for device [" << device.name << "]" << std::endl;
        success = false;
      }
      err = snd_pcm_hw_params_get_buffer_size_max ( hwParams, &maxBufSize ); 
      if (err < 0) {
        std::cerr << "alsa: couldn't get maximum buffer size for device [" << device.name << "]" << std::endl;
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

    /*
    std::vector<AlsaDeviceInfo> scanCards()
    {
      std::vector<AlsaDeviceInfo> devices;

      snd_ctl_card_info_t *card_info;
      snd_pcm_info_t *pcm_info;

      snd_ctl_card_info_alloca( &card_info );
      snd_pcm_info_alloca( &pcm_info );

      for ( int card_index = -1;
            snd_card_next(&card_index) == 0 && card_index >= 0; )
      {
        std::string alsa_card_name = "hw:" + std::to_string(card_index);

        snd_ctl_t *ctl;
        if( snd_ctl_open( &ctl, alsa_card_name.c_str(), 0 ) < 0 )
        {
          std::cerr << "alsa: unable to open sound card " << alsa_card_name << std::endl;
          continue;
        }

        snd_ctl_card_info( ctl, card_info );

        std::string card_name;
        card_name = snd_ctl_card_info_get_name( card_info );

        for ( int dev_index = -1;
              snd_ctl_pcm_next_device(ctl, &dev_index) == 0 && dev_index >= 0; )
        {
          std::string alsa_device_name = alsa_card_name + "," + std::to_string(dev_index);
          std::string device_name;

          // check, if this is a playback device
          snd_pcm_info_set_device( pcm_info, dev_index );
          snd_pcm_info_set_subdevice( pcm_info, 0 );
          snd_pcm_info_set_stream( pcm_info, SND_PCM_STREAM_PLAYBACK );

          if( snd_ctl_pcm_info( ctl, pcm_info ) < 0 )
            continue;

          std::string info_name = SkipCardDetailsInName( (char *)snd_pcm_info_get_name( pcm_info ),
                                                         card_name.c_str() );

          device_name = card_name + ": " + info_name + " (" + alsa_device_name + ")";

          AlsaDeviceInfo device_info;
          device_info.name   = alsa_device_name;
          device_info.descr  = device_name;

          devices.push_back(device_info);
        }

        snd_ctl_close( ctl );
      }

      //snd_pcm_info_free( pcm_info );

      return devices;
    }
    */
    
    std::vector<AlsaDeviceInfo> scanAlsaDevices()
    {
      std::vector<AlsaDeviceInfo> devices;
      
      void **hints, **n;
      char *name, *descr, *descr1, *io;
      
      if (snd_device_name_hint(-1, "pcm", &hints) < 0)
        return {};
      
      n = hints;

      while (*n != NULL)
      {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");

        if (io == NULL || strcmp(io, "Output") == 0)
        {
          if (! ignoreAlsaDevice(name) )
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

    /*
    std::vector<AlsaDeviceInfo> scanPlugins()
    {
      std::vector<AlsaDeviceInfo> devices;

      // updates snd_config by rereading the global configuration files (if needed)
      if( (snd_config) == NULL )
        snd_config_update();

      assert( snd_config );

      int error;

      // iterate over plugin devices
      if ( snd_config_t *top_node = NULL;
           (error = snd_config_search( snd_config, "pcm", &top_node ) >= 0 ) )
      {
        snd_config_iterator_t i, next;

        snd_config_for_each( i, next, top_node )
        {
          const char* tpStr = "unknown";
          const char* idStr = NULL;

          std::string alsa_device_name;
          std::string device_name;
          const AlsaDeviceInfo *predefined = NULL;
          snd_config_t* n = snd_config_iterator_entry( i );
          snd_config_t* tp = NULL;;

          if( (error = snd_config_search( n, "type", &tp )) < 0 )
          {
            if( error != -ENOENT )
              continue;
          }
          else
          {
            snd_config_get_string( tp, &tpStr );
          }

          if (snd_config_get_id( n, &idStr ) < 0)
            continue;

          if( ignorePlugin( idStr ) )
          {
            std::cout << "alsa: ignoring plugin device [" << idStr << "] "
                      << "of type ["<< tpStr << "]" << std::endl;
            continue;
          }

          std::cout << "alsa: found plugin [" << idStr << "] "
                    << "of type [" << tpStr << "]" << std::endl;

          //const AlsaDeviceInfo& predefined = PredefinedDeviceInfo( idStr );

          AlsaDeviceInfo device;
          device.name = idStr;
 
          // if (predefined)
          // {
          //   device.hasPlayback = predefined->hasPlayback;
          //   device.hasCapture = predefined->hasCapture;
          // }
          // else {
          //   device.hasPlayback = 1;
          //   device.hasCapture = 1;
          // }

          devices.push_back(device);
        }
      }
      else
      {
        std::cerr << "alsa: iterating over plugins failed: "
                  << snd_strerror( error ) << std::endl;
      }
      
      return devices;
    }
    */
    
    const microseconds kRequiredLatency = 100000us;

  }//unnamed namespace


  AlsaBackend::AlsaBackend()
    : state_(BackendState::kConfig),
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
    auto alsa_device_list = scanAlsaDevices();

    std::vector<DeviceInfo> device_list;
    device_list.reserve( alsa_device_list.size() );
    
    for (auto& alsa_device : alsa_device_list)
    {
      if ( gropeAlsaDevice(alsa_device) )
      {
        std::cout << alsa_device << std::endl;
        
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
    return device_list;
  }

  void AlsaBackend::configure(const DeviceConfig& config)
  {
    config_ = config;
  }

  DeviceConfig AlsaBackend::open()
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
                               convertSampleFormatToAlsa(config_.spec.format),
                               SND_PCM_ACCESS_RW_INTERLEAVED,
                               config_.spec.channels,
                               config_.spec.rate,
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

    return {};
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

    snd_pcm_sframes_t n_data_frames = bytes / frameSize(config_.spec);
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
      return framesToUsecs(delayp, config_.spec);
  }

  BackendState AlsaBackend::state() const
  {
    return state_;
  }

}//namespace audio
