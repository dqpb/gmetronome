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
#include "Meter.h"

namespace physics {

  using seconds_dbl = std::chrono::duration<double>;

  constexpr seconds_dbl kInfiniteTime {std::numeric_limits<double>::infinity()};
  constexpr seconds_dbl kZeroTime {seconds_dbl::zero()};

  /**
   * @class Kinematics
   * @brief
   */
  class Kinematics { // pure interface
  public:
    virtual ~Kinematics() = default;

    virtual double position() const = 0;
    virtual double velocity() const = 0;
    virtual double acceleration() const = 0;

    virtual void accelerate(double a) = 0;
    virtual void reset(double p, double v, double a) = 0;
    virtual void step(const seconds_dbl& time) = 0;
  };

  /**
   * @class SimpleMotion
   * @brief
   */
  class SimpleMotion : public virtual Kinematics { // impl
  public:
    SimpleMotion(double position = 0.0,
                 double velocity = 0.0,
                 double acceleration = 0.0);

    double position() const override
      { return p_; }
    double velocity() const override
      { return v_; }
    double acceleration() const override
      { return a_; }
    void accelerate(double a) override;

    void reset(double p = 0.0, double v = 0.0, double a = 0.0) override;

    void step(const seconds_dbl& time) override;

  public:
    static constexpr
    std::pair<double,double> step(double position,
                                  double velocity,
                                  double acceleration,
                                  const seconds_dbl& time)
      {
        double v_0 = velocity;

        velocity += acceleration * time.count();
        position += 0.5 * (v_0 + velocity) * time.count();

        return {position, velocity};
      }

  private:
    double p_;
    double v_;
    double a_;
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
    SimpleOscillator(double position = 0.0,
                     double velocity = 0.0,
                     double acceleration = 0.0,
                     double module = 1.0);

    void reset(double p = 0.0, double v = 0.0, double a = 0.0) override;

    void step(const seconds_dbl& time) override;

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
    virtual void syncPosition(double position) = 0;
    virtual void syncVelocity(double velocity) = 0;
  };

  /**
   * @class SimpleSyncOscillator
   * @brief
   */
  class SimpleSyncOscillator : public virtual SyncOscillator, public SimpleOscillator { // impl
  public:
    SimpleSyncOscillator(double position = 0.0,
                         double velocity = 0.0,
                         double acceleration = 0.0,
                         double module = 1.0);

    void syncPosition(double deviation) override;
    void syncVelocity(double deviation) override;

    void accelerate(double a) override;

    void step(const seconds_dbl& time) override;

  private:
    double p_dev_{0.0};
    double v_dev_{0.0};
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
