/*
 * Copyright (C) 2021,2022 The GMetronome Team
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

#include "Synthesizer.h"
#include <algorithm>
#include <random>
#include <future>
#include <cmath>
#include <cassert>

namespace audio {
namespace synth {

  constexpr SampleFormat defaultSampleFormat()
  {
    if (hostEndian() == Endian::kLittle)
      return SampleFormat::kFloat32LE;
    else
      return SampleFormat::kFloat32BE;
  }

  using Sample   = SampleView<defaultSampleFormat(), ByteBuffer::pointer>;
  using Frame    = FrameView<defaultSampleFormat(), ByteBuffer::pointer>;
  using Channel  = ChannelView<defaultSampleFormat(), ByteBuffer::pointer>;
  using Samples  = SampleContainerView<defaultSampleFormat(), ByteBuffer::pointer>;
  using Frames   = FrameContainerView<defaultSampleFormat(), ByteBuffer::pointer>;
  using Channels = ChannelContainerView<defaultSampleFormat(), ByteBuffer::pointer>;

  using ConstSample   = SampleView<defaultSampleFormat(), ByteBuffer::const_pointer>;
  using ConstFrame    = FrameView<defaultSampleFormat(), ByteBuffer::const_pointer>;
  using ConstChannel  = ChannelView<defaultSampleFormat(), ByteBuffer::const_pointer>;
  using ConstSamples  = SampleContainerView<defaultSampleFormat(), ByteBuffer::const_pointer>;
  using ConstFrames   = FrameContainerView<defaultSampleFormat(), ByteBuffer::const_pointer>;
  using ConstChannels = ChannelContainerView<defaultSampleFormat(), ByteBuffer::const_pointer>;

  Samples viewSamples(ByteBuffer& buffer)
  { return audio::viewSamples<defaultSampleFormat()>(buffer); }

  ConstSamples viewSamples(const ByteBuffer& buffer)
  { return audio::viewSamples<defaultSampleFormat()>(buffer); }

  Frames viewFrames(ByteBuffer& buffer)
  { return audio::viewFrames<defaultSampleFormat()>(buffer); }

  ConstFrames viewFrames(const ByteBuffer& buffer)
  { return audio::viewFrames<defaultSampleFormat()>(buffer); }

  Channels viewChannels(ByteBuffer& buffer)
  { return audio::viewChannels<defaultSampleFormat()>(buffer); }

  ConstChannels viewChannels(const ByteBuffer& buffer)
  { return audio::viewChannels<defaultSampleFormat()>(buffer); }

  // pseudo random (-1f, 1f)
  float uniform_distribution()
  {
    static std::uint32_t value = static_cast<std::uint32_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());

    value = value * 214013 + 2531011;
    return 2.0f * (((value & 0x3FFFFFFF) >> 15) / 32767.0) - 1.0f;
  }

  void addNoise(ByteBuffer& buffer, float amplitude)
  {
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    for (auto& frame : frames)
    {
      frame += {
        amplitude * uniform_distribution(),
        amplitude * uniform_distribution()
      };
    }
  }

  // range: (-pi/2,pi/2)
  float bhaskaraCos(float x)
  {
    static constexpr float kPi2 = M_PI * M_PI;
    float x2 = x * x;
    return (kPi2 - 4*x2) / (kPi2 + x2);
  }

  void addSine(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    float arg = -M_PI_2;

    for (auto& frame : frames)
    {
      if (arg > M_PI_2)
        frame -= amplitude * bhaskaraCos(arg - M_PI);
      else
        frame += amplitude * bhaskaraCos(arg);

      arg += omega;
      if (arg > (3.0 * M_PI_2))
        arg = arg - (2.0 * M_PI);
    }
  }

  void addTriangle(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float triangle = 2.0 * std::asin(std::sin(omega * frame_index)) / M_PI;
      frames[frame_index] += amplitude * triangle;
    }
  }

  void addSawtooth(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float time = (omega * frame_index) / (2.0 * M_PI);
      float sawtooth = 2.0 * (time - std::floor(time) - 0.5);
      frames[frame_index] += amplitude * sawtooth;
    }
  }

  void addSquare(ByteBuffer& buffer, float frequency, float amplitude)
  {
    assert(buffer.spec().rate != 0);
    assert(isFloatingPoint(buffer.spec().format));

    auto frames = viewFrames(buffer);
    float omega = 2.0 * M_PI * frequency / buffer.spec().rate;
    for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index)
    {
      float sine = std::sin(omega * frame_index);
      if (sine >= 0)
        frames[frame_index] += amplitude;
      else
        frames[frame_index] -= amplitude;
    }
  }

  void addOscillator(ByteBuffer& buffer, const std::vector<Oscillator>& oscillators)
  {
    for (const auto& osc : oscillators)
    {
      switch (osc.shape) {
      case Waveform::kSine:
        addSine(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kTriangle:
        addTriangle(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kSawtooth:
        addSawtooth(buffer, osc.frequency, osc.amplitude);
        break;
      case Waveform::kSquare:
        addSquare(buffer, osc.frequency, osc.amplitude);
        break;
      }
    }
  }

  void applyAutomation(const ByteBuffer& buffer,
                       const Automation& points,
                       const std::function<void(size_t frame_index, float value)>& fun)
  {
    assert(buffer.spec().rate > 0);

    assert(std::is_sorted(points.begin(), points.end(),
                          [] (const auto& lhs, const auto& rhs)
                            { return lhs.time < rhs.time; }));

    if (points.empty() || buffer.empty())
      return;

    float cur_value = points.front().value;
    size_t cur_frame = 0;

    for (const auto& pt : points)
    {
      size_t pt_frame = usecsToFrames(pt.time, buffer.spec());
      float delta_value = pt.value - cur_value;
      float delta_frames = pt_frame - cur_frame + 1;

      if (delta_frames > 0)
      {
        float slope = delta_value / delta_frames;
        for (; cur_frame <= pt_frame && cur_frame < buffer.frames(); ++cur_frame)
        {
          cur_value += slope;
          fun(cur_frame, cur_value);
        }
      }
    }
   for (; cur_frame < buffer.frames(); ++cur_frame)
     fun(cur_frame, cur_value);
  }

  void applyAutomation(ByteBuffer& buffer,
                       const Automation& points,
                       const std::function<void(Frames& frames, Frame& frame, float value)>& fun)
  {
    auto frames = viewFrames(buffer);
    auto fu = [&frames, &fun] (size_t frame_index, float value)
      { fun(frames, frames[frame_index], value); };

    applyAutomation(buffer, points, fu);
  }

  void applyAutomation(ByteBuffer& buffer,
                       const Automation& points,
                       const std::function<void(Frame& frame, float value)>& fun)
  {
    auto frames = viewFrames(buffer);
    auto fu = [&frames, &fun] (size_t frame_index, float value)
      { fun(frames[frame_index], value); };

    applyAutomation(buffer, points, fu);
  }

  void applyGain(Frame& frame, float gain_l, float gain_r)
  { frame *= {gain_l, gain_r}; }

  void applyGain(Frame& frame, float gain)
  { applyGain(frame, gain, gain); }

  void applyGain(ByteBuffer& buffer, float gain_l, float gain_r)
  {
    assert(buffer.spec().channels == 2);

    auto frames = viewFrames(buffer);
    for (auto& frame : frames)
      applyGain(frame, gain_l, gain_r);
  }

  void applyGain(ByteBuffer& buffer, float gain)
  { applyGain(buffer, gain, gain); }

  void applyGain(ByteBuffer& buffer, const Automation& gain)
  { applyAutomation(buffer, gain, [] (Frame& f, float v) { applyGain(f,v); }); }

  void normalize(ByteBuffer& buffer, float gain_l, float gain_r)
  {
    assert(buffer.spec().channels == 2);

    auto frames = viewFrames(buffer);

    float max = 0.0;
    for (auto& frame : frames)
      max = std::max(max, std::max(std::abs(frame[0]), std::abs(frame[1])));

    for (auto& frame : frames)
      frame *= { gain_l / max, gain_r / max};
  }

  void normalize(ByteBuffer& buffer, float gain)
  { normalize(buffer, gain, gain); }

  void applySmoothing(ByteBuffer& buffer, const Automation& kernel_width)
  {
    auto channels = viewChannels(buffer);

    for (auto& channel : channels)
    {
      applyAutomation(buffer, kernel_width, [&buffer, &channel] (size_t frame_index, float value)
        {
          int kw_half = usecsToFrames(microseconds(int(value)), buffer.spec()) / 2;

          if (kw_half <= 0)
            return;

          auto start = std::max<int>(frame_index - kw_half, 0);
          auto end = std::min<int>(frame_index + kw_half + 1, channel.size());
          float avg = 0;

          for (int i=start; i<end; ++i)
            avg += channel[i];

          channel[frame_index] = avg / (end-start);
        });
    }
  }

  void applySmoothing(ByteBuffer& buffer, microseconds kernel_width)
  { applySmoothing(buffer, {{0ms, float(kernel_width.count())}}); }

  ByteBuffer mixBuffers(ByteBuffer buffer1, const ByteBuffer& buffer2)
  {
    auto frames1 = viewFrames(buffer1);
    auto frames2 = viewFrames(buffer2);

    auto it2 = frames2.begin();
    for (auto it1 = frames1.begin();
         it1 != frames1.end() && it2 != frames2.end();
         ++it1, ++it2)
    {
      *it1 += *it2;
    }
    return buffer1;
  }

  ByteBuffer generateClick(const StreamSpec& spec,
                           float timbre,
                           float pitch,
                           float volume,
                           float balance)
  {
    assert(spec.channels == 2);
    assert(!std::isnan(volume));
    assert(!std::isnan(balance));

    static const milliseconds kBufferDuration = 60ms;

    volume = std::clamp(volume, 0.0f, 1.0f);
    balance = std::clamp(balance, -1.0f, 1.0f);

    float balance_l = (balance > 0) ? -1 * balance + 1 : 1;
    float balance_r = (balance < 0) ?  1 * balance + 1 : 1;

    const StreamSpec buffer_spec =
      {
        defaultSampleFormat(),
        spec.rate,
        spec.channels
      };

    ByteBuffer buffer(buffer_spec, kBufferDuration);

    if (volume > 0)
    {
      ByteBuffer osc_buffer(buffer_spec, kBufferDuration);

      const std::vector<Oscillator> osc = {
        {pitch,         1.0f, Waveform::kSine},
        // {pitch * 0.67f, 0.6f, Waveform::kSine},
        // {pitch * 1.32f, 0.6f, Waveform::kSine},
        // {pitch * 0.43f, 0.3f, Waveform::kSine},

        // {pitch * 1.68f, 0.3f, Waveform::kSine},
        // {pitch * 2.68f, 0.1f, Waveform::kSine}

        {pitch *  3.0f / 2.0f, 0.2f, Waveform::kSine},
        {pitch *  5.0f / 4.0f, 0.1f, Waveform::kSine},
        //{pitch * 15.0f / 8.0f, 0.05f, Waveform::kSquare}
      };

      addOscillator(osc_buffer, osc);

      applyGain(osc_buffer, (timbre + 1.0) / 2.0);

      static const Automation osc_envelope = {
        {0ms,  0.0},
        {3ms,  0.7},
        {7ms,  0.3},
        {8ms,  0.7},
        {12ms, 0.15},
        {30ms, 0.05},
        {60ms, 0.0}
      };
      applyGain(osc_buffer, osc_envelope);

      addNoise(buffer, std::max(-timbre, 0.0f));

      static const Automation noise_smoothing_kw = {
          {0ms,    0.0f},
          {1ms,   20.0f},
          {5ms,   70.0f},
          {6ms,   20.0f},
          {11ms,  70.0f},
          //{30ms,  30.0f},
          //{60ms,  70.0f},
      };
      applySmoothing(buffer, noise_smoothing_kw);

      static const Automation noise_envelope = {
        {0ms,  0.0},
        {1ms,  1.0},
        {5ms,  0.1},
        {6ms,  1.0},
        {11ms, 0.1},
        {60ms, 0.0}
      };
      applyGain(buffer, noise_envelope);

      buffer = mixBuffers(std::move(buffer), osc_buffer);

      // static const Automation final_smoothing = {
      //   {0ms,  0.0f},
      //   {30ms, 0.0f},
      //   {60ms, 200.0f},
      // };
      // applySmoothing(buffer, final_smoothing);

      normalize(buffer, volume * balance_l, volume * balance_r);
    }
    return resample(buffer, spec);
  }

  SoundPackage generateClickPackage(const StreamSpec& spec,
                                    float timbre,
                                    float pitch,
                                    float volume,
                                    float balance,
                                    std::size_t package_size)
  {
    std::vector<std::future<ByteBuffer>> results;
    results.reserve(package_size);

    for (std::size_t p = 0; p < package_size; ++p)
      results.push_back( std::async(std::launch::async,
                                    generateClick,
                                    spec, timbre, pitch, volume, balance) );
    SoundPackage package;
    package.reserve(package_size);

    for (auto& result : results)
      package.push_back( result.get() );

    return package;
  }

}//namespace synth
}//namespace audio
