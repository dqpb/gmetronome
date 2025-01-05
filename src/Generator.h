/*
 * Copyright (C) 2021-2024 The GMetronome Team
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
#include <limits>

namespace audio {

  template<typename... Gs> class StreamController;

  using GeneratorId = size_t;

  constexpr GeneratorId kInvalidGenerator = std::numeric_limits<GeneratorId>::max();

  enum class TempoMode
  {
    kConstant   = 0,
    kContinuous = 1,
    kStepwise   = 2,
    kSync       = 3
  };

  struct StreamStatus
  {
    double       position {0.0};
    double       tempo {0.0};
    TempoMode    mode {TempoMode::kConstant};
    double       acceleration {0.0};
    int          hold {0};
    int          accent {0};
    microseconds next_accent_delay {0us};
    GeneratorId  generator {kInvalidGenerator};
  };

  /**
   * @class StreamGenerator
   * @brief A state in a StreamController.
   */
  template<typename Controller>
  class StreamGenerator {
  public:
    virtual void onTempoChanged(Controller& ctrl, TempoMode old_mode) {}
    virtual void onMeterChanged(Controller& ctrl, const Meter& old_meter, bool enabled_changed) {}
    virtual void onSoundChanged(Controller& ctrl, Accent a) {}
    virtual void onStart(Controller& ctrl) {}
    virtual void onStop(Controller& ctrl) {}

    virtual void prepare(Controller& ctrl) {}
    virtual void enter(Controller& ctrl) {}
    virtual void leave(Controller& ctrl) {}
    virtual void cycle(Controller& ctrl, const void*& data, size_t& bytes) {}

    virtual void updateStatus(Controller& ctrl, StreamStatus& status) {}

  protected:
    void switchGenerator(Controller& ctrl, GeneratorId gen);

    template<GeneratorId I>
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
    explicit StreamController(const StreamSpec& spec = kDefaultSpec);

    void setTempo(double tempo);
    void accelerate(double accel, double target);
    void accelerate(int hold, double step, double target);
    void synchronize(double beats, double tempo, microseconds time);
    void swapMeter(Meter& meter);
    void resetMeter();
    void setSound(Accent accent, const SoundParameters& params);

    double tempo() const
      { return tempo_; }
    TempoMode mode() const
      { return mode_; }
    double target() const
      { return target_; }
    double acceleration() const
      { return accel_; }
    int hold() const
      { return hold_; }
    double step() const
      { return step_; }
    double syncBeats() const
      { return sync_beats_; }
    double syncTempo() const
      { return sync_tempo_; }
    microseconds syncTime() const
      { return sync_time_; }
    const StreamSpec& spec() const
      { return spec_; }
    const Meter& meter() const
      { return meter_; }
    const bool isMeterEnabled() const
      { return meter_enabled_; }
    const ByteBuffer& sound(Accent a)
      { return sounds_[a]; }

    void prepare(const StreamSpec& spec);

    void start(GeneratorId gen);
    void stop();
    void cycle(const void*& data, size_t& bytes);

    const StreamStatus& status();

  private:
    StreamGeneratorTuple gs_;
    StreamSpec spec_;
    double tempo_{0.0};
    TempoMode mode_{TempoMode::kConstant};
    double target_{0.0};
    double accel_{0.0};
    int    hold_{0};
    double step_{0.0};
    double sync_beats_{0.0};
    double sync_tempo_{0.0};
    microseconds sync_time_{0};
    Meter default_meter_{kMeter1};
    Meter meter_{kMeter1};
    bool meter_enabled_{false};
    SoundLibrary sounds_;
    StreamStatus stream_status_;
    StreamGeneratorBase* g_{nullptr};

    friend StreamGeneratorBase;

    template<size_t N=0>
    void switchGenerator(GeneratorId gen);

    template<GeneratorId I>
    auto& generator();
  };

  //
  // StreamGenerator
  //
  class StreamGeneratorError : public GMetronomeError {
  public:
    explicit StreamGeneratorError(const std::string& what = "") : GMetronomeError(what)
      { }
  };

  template<typename Controller>
  void StreamGenerator<Controller>::switchGenerator(Controller& ctrl, GeneratorId gen)
  {
    ctrl.switchGenerator(gen);
  }

  template<typename Controller>
  template<GeneratorId I>
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
    explicit StreamControllerError(const std::string& what = "") : GMetronomeError(what)
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
    TempoMode old_mode = mode_;

    tempo_ = tempo;
    mode_ = TempoMode::kConstant;

    if (g_) g_->onTempoChanged(*this, old_mode);
  }

  template<typename...Gs>
  void StreamController<Gs...>::accelerate(double accel, double target)
  {
    TempoMode old_mode = mode_;

    accel_ = accel;
    target_ = target;
    mode_ = TempoMode::kContinuous;

    if (g_) g_->onTempoChanged(*this, old_mode);
  }

  template<typename...Gs>
  void StreamController<Gs...>::accelerate(int hold, double step, double target)
  {
    TempoMode old_mode = mode_;

    hold_ = hold;
    step_ = step;
    target_ = target;
    mode_ = TempoMode::kStepwise;

    if (g_) g_->onTempoChanged(*this, old_mode);
  }

  template<typename...Gs>
  void StreamController<Gs...>::synchronize(double beats, double tempo, microseconds time)
  {
    TempoMode old_mode = mode_;

    sync_beats_ = beats;
    sync_tempo_ = tempo;
    sync_time_ = time;
    mode_ = TempoMode::kSync;

    if (g_) g_->onTempoChanged(*this, old_mode);
  }

  template<typename...Gs>
  void StreamController<Gs...>::swapMeter(Meter& meter)
  {
    bool enabled_changed = !meter_enabled_;
    if (!meter_enabled_)
    {
      std::swap(default_meter_, meter_);
      meter_enabled_ = true;
    }
    std::swap(meter_, meter);
    if (g_) g_->onMeterChanged(*this, meter, enabled_changed);
  }

  template<typename...Gs>
  void StreamController<Gs...>::resetMeter()
  {
    if (meter_enabled_)
    {
      std::swap(meter_, default_meter_);
      meter_enabled_ = false;
      if (g_) g_->onMeterChanged(*this, default_meter_, true);
    }
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
  void StreamController<Gs...>::start(GeneratorId gen)
  {
    switchGenerator(gen);
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
  void StreamController<Gs...>::switchGenerator(GeneratorId gen)
  {
    if (N == gen) {
      auto* new_g = &std::get<N>(gs_);
      if (g_) g_->leave(*this);
      g_ = new_g;
      if (g_) g_->enter(*this);
    }
    if constexpr (N + 1 < std::tuple_size_v<StreamGeneratorTuple>) {
      return switchGenerator<N + 1>(gen);
    }
  }

  template<typename...Gs>
  template<GeneratorId I>
  auto& StreamController<Gs...>::generator()
  {
    return std::get<I>(gs_);
  }

  class FillBufferGenerator;
  class PreCountGenerator;
  class RegularGenerator;
  class DrainBufferGenerator;

  using BeatStreamController = StreamController<
    FillBufferGenerator,
    PreCountGenerator,
    RegularGenerator,
    DrainBufferGenerator
    >;

  constexpr GeneratorId kFillBufferGenerator  = 0;
  constexpr GeneratorId kPreCountGenerator    = 1;
  constexpr GeneratorId kRegularGenerator     = 2;
  constexpr GeneratorId kDrainBufferGenerator = 3;

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
    void onTempoChanged(BeatStreamController& ctrl, TempoMode old_mode) override;
    void onMeterChanged(BeatStreamController& ctrl,
                        const Meter& old_meter, bool enabled_changed) override;

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
    int hold_{0};

    void updateFramesLeft(BeatStreamController& ctrl);
    void step(BeatStreamController& ctrl, size_t frames_chunk);

    void recomputeStepwise(BeatStreamController& ctrl);
    void resetStepwise(BeatStreamController& ctrl);
    void handleStepwise(BeatStreamController& ctrl);
    void accelerateStepwise(BeatStreamController& ctrl);
    TempoMode effectiveMode(const BeatStreamController& ctrl) const;
  };

  /**
   * @class DrainBufferGenerator
   */
  class DrainBufferGenerator : public StreamGenerator<BeatStreamController> {
  public:
    void cycle(BeatStreamController& ctrl, const void*& data, size_t& bytes) override;
  };

  /**
   * @class StreamTimer
   */
  class StreamTimer {
  public:
    void start(microseconds time)
      { running_ = true; bytes_ = usecsToBytes(time, spec_); }
    bool finished() const
      { return running_ && bytes_ == 0; }
    bool running() const
      { return running_; }
    void step(size_t bytes)
      { bytes_ = (bytes < bytes_) ? bytes_-bytes : 0; }
    microseconds remaining() const
      { return bytesToUsecs(bytes_, spec_); }
    void reset()
      { running_ = false; bytes_ = 0; }
    void switchStreamSpec(const StreamSpec& spec)
      {
        if (bytes_ != 0) bytes_ = usecsToBytes(remaining(), spec);
        spec_ = spec;
      }
  private:
    StreamSpec spec_{kDefaultSpec};
    bool running_{false};
    size_t bytes_{0};
  };

}//namespace audio
#endif//GMetronome_Generator_h
