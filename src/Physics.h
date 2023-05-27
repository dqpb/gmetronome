/*
 * Copyright (C) 2023 The GMetronome Team
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

#ifndef GMetronome_Physics_h
#define GMetronome_Physics_h

#include <chrono>
#include <limits>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "Meter.h"

namespace physics {

  using seconds_dbl = std::chrono::duration<double>;

  constexpr seconds_dbl kInfiniteTime {std::numeric_limits<double>::infinity()};
  constexpr seconds_dbl kZeroTime {seconds_dbl::zero()};

  using std::literals::chrono_literals::operator""s;

  // position, velocity, acceleration
  using MotionParameters = std::array<double,3>;


  template<typename... Ms>
  class Kin {
  public:
    Kin(const MotionParameters& params = {0.0, 0.0, 0.0})
      : params_{params}
      {}

    const MotionParameters& state() const
      { return params_; }

    void reset(const MotionParameters& params)
      { params_ = params; }

    void step(const seconds_dbl& time)
      {
        std::apply(
          [&] (auto&&... mods) {((mods.step(params_, time)), ...);}, mods_);
      }

    std::tuple<Ms...>& modifiers()
      { return mods_; }

  private:
    MotionParameters params_;
    std::tuple<Ms...> mods_;
  };


  class ApplyAcceleration {
  public:
    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v,a] = params;
        double v_0 = v;

        v += a * time.count();
        p += 0.5 * (v_0 + v) * time.count();
      }

  };

  class SyncVelocity {
  public:
    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v,a] = params;

        if (sync_init_)
        {
          if (remaining_time_ == kZeroTime)
            v += v_;
          else
            a += a_;

          sync_init_ = false;
        }

        if (remaining_time_ > kZeroTime)
        {

          if (remaining_time_ >= time)
          {

          }
        }
      }

    void sync(double deviation, const seconds_dbl& time)
      {
        if (time != kZeroTime)
          a_ = deviation / time.count();
        else
          v_ = deviation;

        remaining_time_ = time;
        sync_init_ = true;
      }

  private:
    union {double v_; double a_;};
    seconds_dbl remaining_time_{0s};
    bool sync_init_{false};
  };

  class ModuloModifier {
  public:
    ModuloModifier(double m = 1.0) : m_{m}
      { assert(m != 0.0); }

    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v,a] = params;
        p = modulo(p, m_);
      }

    double module() const
      { return m_; }

    void remodule(double m)
      {
        assert(m != 0.0);
        m_ = m;
      }

    static constexpr double modulo(double x, double y)
      {
        return x - y * std::floor( x / y );
      }

  private:
    double m_;
  };











  /**
   * @class Kinematics
   * @brief
   */
  class Kinematics { // pure interface
  public:
    virtual ~Kinematics() = default;

    virtual const MotionParameters& state() const = 0;
    virtual void set(const MotionParameters& params) = 0;
    virtual void step(const seconds_dbl& time) = 0;
  };

  /**
   * @class SimpleMotion
   * @brief
   */
  class SimpleMotion : public virtual Kinematics { // impl
  public:
    SimpleMotion(const MotionParameters& params = {0.0, 0.0, 0.0});

    const MotionParameters& state() const override
      { return params_; }
    void set(const MotionParameters& params) override;
    void step(const seconds_dbl& time) override;

  private:
    MotionParameters params_;
  };

  /**
   * @class Oscillator
   * @brief
   */
  class Oscillator : public virtual Kinematics { // pure interface
  public:
    virtual ~Oscillator() = default;

    virtual double module() const = 0;
    virtual void remodule(double m) = 0;
  };

  /**
   * @class SimpleOscillator
   * @brief
   */
  class SimpleOscillator : public virtual Oscillator, public SimpleMotion { // impl
  public:
    SimpleOscillator(const MotionParameters& params = {0.0, 0.0, 0.0}, double module = 1.0);

    // Kinematics
    void set(const MotionParameters& params) override;
    void step(const seconds_dbl& time) override;

    // Oscillator
    double module() const override
      { return m_; }
    void remodule(double m) override;

  private:
    double m_;
  };

  /**
   * @class SyncOscillator
   * @brief Synchronizable oscillator
   */
  class SyncOscillator : public virtual Oscillator { // pure interface
  public:
    virtual void syncPosition(double deviation, const seconds_dbl& time) = 0;
    virtual void syncVelocity(double deviation, const seconds_dbl& time) = 0;
    virtual bool isSyncingPosition() const = 0;
    virtual bool isSyncingVelocity() const = 0;
  };

  /**
   * @class SimpleSyncOscillator
   * @brief
   */
  class SimpleSyncOscillator : public virtual SyncOscillator, public SimpleOscillator { // impl
  public:
    SimpleSyncOscillator(const MotionParameters& params = {0.0, 0.0, 0.0}, double module = 1.0);

    // Kinematics
    void step(const seconds_dbl& time) override;

    // SyncOscillator
    void syncPosition(double deviation, const seconds_dbl& time) override;
    void syncVelocity(double deviation, const seconds_dbl& time) override;
    bool isSyncingPosition() const override;
    bool isSyncingVelocity() const override;

  private:
    MotionParameters p_params_{0.0, 0.0, 0.0};
    double p_m_;
    double p_m_t1_;
    seconds_dbl p_time_{0s};

    // sync velocity
    double v_a_{0.0};
    seconds_dbl v_time_{0s};
  };

  // class Metronome : public virtual SyncOscillator, public SimpleOscillator { // impl
  // public:
  //   /**
  //    * @function changeTempo
  //    * @brief Set the current tempo of the oscillation
  //    *
  //    * Setting the current tempo does not affect the target tempo and
  //    * the target acceleration, i.e. the oscillator will continue to
  //    * change the speed towards the target tempo.
  //    */
  //   void changeTempo(double tempo);

  //   /**
  //    * @function setTargetTempo
  //    * @brief Sets the target tempo of the oscillation
  //    *
  //    */
  //   void setTargetTempo(double target_tempo);

  //   /**
  //    * @function setTargetAcceleration
  //    * @brief Sets the acceleration used to reach the target tempo
  //    *
  //    */
  //   void setTargetAcceleration(double accel);

  //   /**
  //    * @function switchMeter
  //    */
  //   void switchMeter(Meter meter);

  //   /**
  //    * @function syncPosition
  //    */
  //   void syncPosition(double beat);

  //   /**
  //    * @function step
  //    */
  //   void step(const seconds_dbl& time);

  //   using Oscillator::position;

  //   double velocity() const
  //     { return Oscillator::velocity() * 60.0; }

  //   double acceleration() const
  //     { return Oscillator::acceleration() * 60.0 * 60.0; }

  // private:
  //   double target_tempo_{0.0};
  //   double target_acceleration_{0.0};
  //   double phase_deviation_{0.0};
  //   Meter meter_{kMeter1};
  // };

}//namespace physics
#endif//GMetronome_Physics_h
