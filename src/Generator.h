/*
 * Copyright (C) 2021-2023 The GMetronome Team
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

#ifndef GMetronome_Generator_h
#define GMetronome_Generator_h

#include "AudioBuffer.h"
#include "SoundLibrary.h"
#include "Meter.h"
#include "Physics.h"
#include "Error.h"

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <string>

namespace audio {

  template<typename... Gs> class StreamController;

  struct StreamStatus
  {
    double position {0.0};
    double tempo {0.0};
    double acceleration {0.0};
    double module {1.0};
    int next_accent {0};
    microseconds next_accent_delay {0us};
    size_t state {0};
  };

  /**
   * @class StreamGenerator
   * @brief A state in a StreamController.
   */
  template<typename Controller>
  class StreamGenerator {
  public:
    virtual void onTempoChanged(Controller& ctrl) {}
    virtual void onTargetTempoChanged(Controller& ctrl) {}
    virtual void onAccelerationChanged(Controller& ctrl) {}
    virtual void onSynchronize(Controller& ctrl, double beat_dev, double tempo_dev) {}
    virtual void onMeterChanged(Controller& ctrl) {}
    virtual void onSoundChanged(Controller& ctrl, Accent a) {}
    virtual void onStart(Controller& ctrl) {}
    virtual void onStop(Controller& ctrl) {}

    virtual void prepare(Controller& ctrl) {}

    virtual void enter(Controller& ctrl) {}
    virtual void leave(Controller& ctrl) {}
    virtual void cycle(Controller& ctrl, const void*& data, size_t& bytes) {}

    virtual void updateStatus(Controller& ctrl, StreamStatus& status) {}

  protected:
    void switchGenerator(Controller& ctrl, size_t index);

    template<size_t I>
    auto& generator(Controller& ctrl);
  };

  /**
   * @class StreamController
   */
  template<typename... Gs>
  class StreamController {
  public:
    using StreamGeneratorBase = StreamGenerator<StreamController<Gs...>>;
    using StreamGeneratorTuple = std::tuple<Gs...>;

    static_assert((std::is_base_of_v<StreamGeneratorBase, Gs> && ...),
                  "Invalid StreamGenerator type");
  public:
    StreamController(const StreamSpec& spec = kDefaultSpec);

    void setTempo(double tempo);
    void setTargetTempo(double tempo);
    void setAcceleration(double accel);
    void synchronize(double beat_dev, double tempo_dev);
    void swapMeter(Meter& meter);
    void setSound(Accent accent, const SoundParameters& params);

    double tempo() const
      { return tempo_; }
    double targetTempo() const
      { return target_tempo_; }
    double acceleration() const
      { return accel_; }
    const StreamSpec& spec() const
      { return spec_; }
    const Meter& meter() const
      { return meter_; }
    const ByteBuffer& sound(Accent a)
      { return sounds_[a]; }

    void prepare(const StreamSpec& spec);

    void start(size_t index);
    void stop();
    void cycle(const void*& data, size_t& bytes);

    const StreamStatus& status();

  private:
    StreamGeneratorTuple gs_;
    StreamSpec spec_;
    double tempo_{0.0};
    double target_tempo_{0.0};
    double accel_{0.0};
    Meter meter_{kMeter1};
    SoundLibrary sounds_;
    StreamStatus stream_status_;
    StreamGeneratorBase* g_{nullptr};

    friend StreamGeneratorBase;

    template<size_t N=0>
    void switchGenerator(size_t index);

    template<size_t I>
    auto& generator();
  };

  //
  // StreamGenerator
  //
  class StreamGeneratorError : public GMetronomeError {
  public:
    StreamGeneratorError(const std::string& what = "") : GMetronomeError(what)
      { }
  };

  template<typename Controller>
  void StreamGenerator<Controller>::switchGenerator(Controller& ctrl, size_t index)
  {
    ctrl.switchGenerator(index);
  }

  template<typename Controller>
  template<size_t I>
  auto& StreamGenerator<Controller>::generator(Controller& ctrl)
  {
    return ctrl.template generator<I>();
  }

  //
  // StreamController
  //
  class StreamControllerError : public GMetronomeError
  {
  public:
    StreamControllerError(const std::string& what = "") : GMetronomeError(what)
      { }
  };

  template<typename...Gs>
  StreamController<Gs...>::StreamController(const StreamSpec& spec)
    : spec_{spec}
  {
    SoundParameters silence;
    silence.volume = 0.0;

    sounds_.insert(kAccentOff, silence);
    sounds_.insert(kAccentWeak, SoundParameters{});
    sounds_.insert(kAccentMid, SoundParameters{});
    sounds_.insert(kAccentStrong, SoundParameters{});
    sounds_.prepare(spec_);
  }

  template<typename...Gs>
  void StreamController<Gs...>::setTempo(double tempo)
  {
    tempo_ = tempo;
    if (g_) g_->onTempoChanged(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::setTargetTempo(double tempo)
  {
    target_tempo_ = tempo;
    if (g_) g_->onTargetTempoChanged(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::setAcceleration(double accel)
  {
    accel_ = accel;
    if (g_) g_->onAccelerationChanged(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::synchronize(double beat_dev, double tempo_dev)
  {
    if (g_) g_->onSynchronize(*this, beat_dev, tempo_dev);
  }

  template<typename...Gs>
  void StreamController<Gs...>::swapMeter(Meter& meter)
  {
    std::swap(meter_, meter);
    if (g_) g_->onMeterChanged(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::setSound(Accent accent, const SoundParameters& params)
  {
    sounds_.update(accent, params);
    if (g_) g_->onSoundChanged(*this, accent);
  }

  template<typename...Gs>
  void StreamController<Gs...>::prepare(const StreamSpec& spec)
  {
    assert(spec.rate > 0);

    if (spec != spec_)
    {
      sounds_.prepare(spec);

      // Since a change of the stream specification may necessitate resizing
      // the sound buffers we update the sounds immediately to prevent memory
      // allocations during real-time processing.
      sounds_.apply();

      spec_ = spec;
    }

    std::apply( [this] (auto&&... args) { (args.prepare(*this), ...);}, gs_ );
  }

  template<typename...Gs>
  void StreamController<Gs...>::start(size_t index)
  {
    switchGenerator(index);
    if (g_) g_->onStart(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::stop()
  {
    if (g_) g_->onStart(*this);
  }

  template<typename...Gs>
  void StreamController<Gs...>::cycle(const void*& data, size_t& bytes)
  {
    if (g_) g_->cycle(*this, data, bytes);
  }

  template<typename...Gs>
  const StreamStatus& StreamController<Gs...>::status()
  {
    if (g_) g_->updateStatus(*this, stream_status_);
    return stream_status_;
  }

  template<typename...Gs>
  template<size_t N>
  void StreamController<Gs...>::switchGenerator(size_t index)
  {
    if (N == index) {
      auto* new_g = &std::get<N>(gs_);
      if (g_) g_->leave(*this);
      g_ = new_g;
      if (g_) g_->enter(*this);
    }
    if constexpr (N + 1 < std::tuple_size_v<StreamGeneratorTuple>) {
      return switchGenerator<N + 1>(index);
    }
  }

  template<typename...Gs>
  template<size_t I>
  auto& StreamController<Gs...>::generator()
  {
    return std::get<I>(gs_);
  }

  class FillBufferGenerator;
  class PreCountGenerator;
  class RegularGenerator;
  class DrainGenerator;

  using BeatStreamController = StreamController<
    FillBufferGenerator,
    PreCountGenerator,
    RegularGenerator,
    DrainGenerator
    >;

  constexpr size_t kFillBufferGenerator = 0;
  constexpr size_t kPreCountGenerator = 1;
  constexpr size_t kRegularGenerator = 2;
  constexpr size_t kDrainGenerator = 3;

  /**
   * @class FillBufferGenerator
   */
  class FillBufferGenerator : public StreamGenerator<BeatStreamController> {
  public:
    void prepare(BeatStreamController& ctrl) override;

    void enter(BeatStreamController& ctrl) override;
    void leave(BeatStreamController& ctrl) override;
    void cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes) override;

    void updateStatus(BeatStreamController& ctrl, StreamStatus& status) override;

  private:
    size_t max_chunk_frames_{0};
    size_t avg_chunk_frames_{0};
    size_t frames_total_{0};
    size_t frames_done_{0};
  };

  /**
   * @class PreCountGenerator
   */
  class PreCountGenerator : public StreamGenerator<BeatStreamController> {
  public:
    void enter(BeatStreamController& ctrl) override;
    void leave(BeatStreamController& ctrl) override;
  };

  /**
   * @class RegularGenerator
   */
  class RegularGenerator : public StreamGenerator<BeatStreamController> {
  public:
    void onTempoChanged(BeatStreamController& ctrl) override;
    void onTargetTempoChanged(BeatStreamController& ctrl) override;
    void onAccelerationChanged(BeatStreamController& ctrl) override;
    void onSynchronize(BeatStreamController& ctrl, double beat_dev, double tempo_dev) override;
    void onMeterChanged(BeatStreamController& ctrl) override;
    void onSoundChanged(BeatStreamController& ctrl, Accent a) override;
    void onStart(BeatStreamController& ctrl) override;
    void onStop(BeatStreamController& ctrl) override;

    void prepare(BeatStreamController& ctrl) override;

    void enter(BeatStreamController& ctrl) override;
    void leave(BeatStreamController& ctrl) override;
    void cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes) override;

    void updateStatus(BeatStreamController& ctrl, StreamStatus& status) override;

  private:
    physics::BeatKinematics k_;
    size_t max_chunk_frames_{0};
    size_t avg_chunk_frames_{0};
    size_t accent_{0};
    size_t frames_left_{0};
    bool accent_point_{false};

    void updateFramesLeft(BeatStreamController& ctrl);
    void step(BeatStreamController& ctrl, size_t frames_chunk);
  };

  /**
   * @class DrainGenerator
   */
  class DrainGenerator : public StreamGenerator<BeatStreamController> {
  public:
    void cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes) override;
  };

}//namespace audio
#endif//GMetronome_Generator_h
