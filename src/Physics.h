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

#include "Auxiliary.h"

#include <chrono>
#include <limits>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>

namespace physics {

  using seconds_dbl = std::chrono::duration<double>;

  constexpr seconds_dbl kInfiniteTime {std::numeric_limits<double>::infinity()};
  constexpr seconds_dbl kZeroTime {seconds_dbl::zero()};

  /**
   * @class ConstantForce
   */
  struct ConstantForce {
    double base {0.0};

    void add(const ConstantForce& f)
      { base += f.base; }

    void reset()
      { base = 0.0; }

    void apply(double& p, double& v, const seconds_dbl& time)
      {
        double t1 = time.count();
        double t2 = t1 * t1;

        p += base / 2.0 * t2 + v * t1;
        v += base * t1;
      }

    void shift(const seconds_dbl& time)
      { /* nothing */ }
  };

  /**
   * @class LinearForce
   * @brief A force that changes linearly over time
   */
  struct LinearForce {
    double base {0.0};
    double slope {0.0};

    void add(const LinearForce& f)
      {
        base += f.base;
        slope += f.slope;
      }

    void reset()
      {
        base = 0.0;
        slope = 0.0;
      }

    void apply(double& p, double& v, const seconds_dbl& time)
      {
        double t1 = time.count();
        double t2 = t1 * t1;
        double t3 = t2 * t1;

        p += slope / 6.0 * t3 + base / 2.0 * t2 + v * t1;
        v += slope / 2.0 * t2 + base * t1;
      }

    void shift(const seconds_dbl& time)
      { base += slope * time.count(); }

    seconds_dbl arrival(double& p0, double& v0, double p)
      {
      }
  };

  /**
   * @class Oscillator
   * @brief Simulate a rotating point mass under the influence of multiple forces
   *
   * An Oscillator simulates the kinematics of the trajectory of a rotating point
   * mass under the influence of multiple (possibly) time varying forces.
   */
  template<typename ForceType = ConstantForce>
  class Oscillator {
  public:
    Oscillator(double module = 1.0) : m_{module} {}

    void remodule(double m)
      {
        assert(m != 0.0);
        m_ = m;
        p_ = aux::math::modulo(p_,m_);
      }

    double position() const
      { return p_; }
    double velocity() const
      { return v_; }
    void reset(double p, double v)
      { p_ = p; v_ = v; }

    // net force currently acting upon the point mass
    const ForceType& force() const
      { return f_.force; }

    // remaining time before force composition changes
    const seconds_dbl& remaining() const
      { return f_.time; }

    // whether a force contributes to the current net force
    bool contributes(size_t id) const
      {
        assert(id < forces_.size());
        return (forces_[id].time > kZeroTime);
      }

    size_t registerForce (ForceType f, const seconds_dbl& time = kInfiniteTime)
      {
        forces_.push_back({std::move(f), time});
        addForceTime(forces_.back());
        return forces_.size() - 1;
      }

    void updateForce(size_t id, ForceType f, const seconds_dbl& time)
      {
        assert(id < forces_.size());
        forces_[id].force = std::move(f);
        forces_[id].time = time;
        updateForceTime();
      }

    void resetForce(size_t id)
      {
        assert(id < forces_.size());
        forces_[id].force.reset();
        forces_[id].time = kZeroTime;
        updateForceTime();
      }

    void step(seconds_dbl time)
      {
        while (time > kZeroTime)
        {
          seconds_dbl force_time = std::min(time, f_.time);

          if (force_time == kZeroTime)
            force_time = time;

          f_.force.apply(p_,v_,force_time);

          p_ = aux::math::modulo(p_,m_);

          updateForceTime(force_time);

          time -= force_time;
        }
      }

  private:
    double m_;
    double p_{0.0};
    double v_{0.0};

    struct ForceTime_
    {
      ForceType force;              // current net force
      seconds_dbl time {kZeroTime}; // remainng time for net force
    };

    std::vector<ForceTime_> forces_;

    ForceTime_ f_{{}, kInfiniteTime};

    void resetForceTime()
      {
        f_.force.reset();
        f_.time = kInfiniteTime;
      }

    void updateForceTime(const seconds_dbl& time = kZeroTime)
      {
        resetForceTime();
        for (auto& f : forces_)
        {
          if (time < f.time)
          {
            f.force.shift(time);
            f.time -= time;
          }
          else
          {
            f.force.shift(f.time);
            f.time = kZeroTime;
          }
          addForceTime(f);
        }
      }

    void addForceTime(const ForceTime_& f)
      {
        if (f.time > kZeroTime)
        {
          f_.force.add(f.force);
          f_.time = std::min(f_.time, f.time);
        }
      }
  };



  class Metronome {
  public:
    /**
     * @function changeTempo
     * @brief Set the current tempo of the oscillation
     *
     * Setting the current tempo does not affect the target tempo and
     * the target acceleration, i.e. the oscillator will continue to
     * change the speed towards the target tempo.
     */
    void changeTempo(double tempo);

    /**
     * @function setTargetTempo
     * @brief Sets the target tempo of the oscillation
     */
    void setTargetTempo(double target_tempo);

    /**
     * @function setTargetAcceleration
     * @brief Sets the acceleration used to reach the target tempo
     */
    void setTargetAcceleration(double accel);

    /**
     * @function switchMeter
     */
    //void switchMeter(Meter meter);

    /**
     * @function syncPosition
     */
    void syncPosition(double beat);

    /**
     * @function syncTempo
     */
    void syncTempo(double tempo);

    double position() const
      { return osc_.position(); }

    double velocity() const
      { return osc_.velocity(); }

    double acceleration() const
      { return osc_.force().base; }

  private:
    Oscillator<LinearForce> osc_;
  };




  /*
  template<typename... Ms>
  class Kin {
  public:
    Kin(const MotionParameters& params = {0.0, 0.0})
      : params_{params}
      {}

    const MotionParameters& state() const
      { return params_; }

    double acceleration() const
      {
        double a = 0.0;
        std::apply(
          [&] (auto&&... mods) {((a += mods.acceleration()), ...);}, mods_);
        return a;
      }

    void reset(const MotionParameters& params)
      { params_ = params; }

    void step(const seconds_dbl& time)
      {
        std::apply(
          [&] (auto&&... mods) {((mods.step(params_, time)), ...);}, mods_);
      }

    const std::tuple<Ms...>& modifiers() const
      { return mods_; }
    std::tuple<Ms...>& modifiers()
      { return mods_; }

    template<size_t I> const auto& modifier() const
      { return std::get<I>(mods_); }
    template<size_t I> auto& modifier()
      { return std::get<I>(mods_); }
    template<typename T> const auto& modifier() const
      { return std::get<T>(mods_); }
    template<typename T> auto& modifier()
      { return std::get<T>(mods_); }

  private:
    MotionParameters params_;
    std::tuple<Ms...> mods_;
  };


  class SimpleAcceleration {
  public:
    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v] = params;

        SimpleMotion::apply(params, a_, time); }

    double acceleration() const
      { return a_; }

    void accelerate(double a)
      { a_ = a; };

    template<typename TupleType>
    static constexpr
    void apply(TupleType& params, double accel, const seconds_dbl& time)
      {
        auto& [p,v] = params;
        double v_0 = v;

        v += accel * time.count();
        p += 0.5 * (v_0 + v) * time.count();
      }
  private:
    double a_{0.0};

  // public:
  //   void step(MotionParameters& params, const seconds_dbl& time)
  //     {
  //       auto& [p,v,a] = params;

  //       if (change_a_)
  //       {
  //         a = a - a_ + new_a_;
  //         a_ = new_a_;
  //         change_a_ = false;
  //       }

  //       std::tuple<double&,double&,double> tmp{p, v, a_};
  //       SimpleMotion::apply(params, time);
  //     }

  //   void accelerate(double a)
  //     {
  //       new_a_ = a;
  //       change_a_ = true;
  //     };

  //   template<typename TupleType>
  //   static constexpr
  //   void apply(TupleType& params, const seconds_dbl& time)
  //     {
  //       auto& [p,v,a] = params;
  //       double v_0 = v;

  //       v += a * time.count();
  //       p += 0.5 * (v_0 + v) * time.count();
  //     }
  // private:
  //   double a_{0.0};
  //   double new_a_{0.0};
  //   bool change_a_{false};
  };


  class SyncVelocity {
  public:
    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v] = params;

        if (sync_init_)
        {
          if (remaining_time_ == kZeroTime)
            v += v_;

          sync_init_ = false;
        }

        if (remaining_time_ > kZeroTime)
        {
          auto& accel_time = std::min(time, remaining_time_);
          SimpleMotion::apply(params, a_, accel_time);

          remaining_time_ -= accel_time;
          if (remaining_time_ <= kZeroTime)
            a_ = 0.0;
        }
      }

    double acceleration() const
      { return a_; }

    void sync(double deviation, const seconds_dbl& time)
      {
        if (time != kZeroTime)
        {
          a_ = deviation / time.count();
          v_ = 0.0;
        }
        else
        {
          a_ = 0.0;
          v_ = deviation;
        }
        remaining_time_ = time;
        sync_init_ = true;
      }

  private:
    double v_{0.0};
    double a_{0.0};
    seconds_dbl remaining_time_{0s};
    bool sync_init_{false};


// public:
  //   void step(MotionParameters& params, const seconds_dbl& time)
  //     {
  //       auto& [p,v,a] = params;

  //       if (sync_init_)
  //       {
  //         if (remaining_time_ == kZeroTime)
  //           v += v_;
  //         else
  //           a += a_;

  //         sync_init_ = false;
  //       }

  //       if (remaining_time_ > kZeroTime)
  //       {
  //         std::tuple<double&,double&,double> tmp{p, v_, a_};
  //         auto& accel_time = std::min(time, remaining_time_);
  //         SimpleMotion::apply(tmp, accel_time);

  //         remaining_time_ -= accel_time;
  //         if (remaining_time_ <= kZeroTime)
  //         {
  //           a -= a_;
  //           a_ = 0.0;
  //         }
  //       }
  //     }

  //   void sync(double deviation, const seconds_dbl& time)
  //     {
  //       if (time != kZeroTime)
  //       {
  //         a_ = deviation / time.count();
  //         v_ = 0.0;
  //         if (remaining_time_ > kZeroTime)
  //         {
  //           //a_ -=
  //         }
  //       }
  //       else
  //       {
  //         v_ = deviation;
  //       }
  //       remaining_time_ = time;
  //       sync_init_ = true;
  //     }

  // private:
  //   double v_{0.0};
  //   double a_{0.0};
  //   seconds_dbl remaining_time_{0s};
  //   bool sync_init_{false};
  };

  class ModuloModifier {
  public:
    ModuloModifier(double m = 1.0) : m_{m}
      { assert(m != 0.0); }

    void step(MotionParameters& params, const seconds_dbl& time)
      {
        auto& [p,v] = params;
        p = modulo(p, m_);
      }

    double acceleration() const
      { return 0.0; }

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

  */









  // /**
  //  * @class Kinematics
  //  * @brief
  //  */
  // class Kinematics { // pure interface
  // public:
  //   virtual ~Kinematics() = default;

  //   virtual const MotionParameters& state() const = 0;
  //   virtual void set(const MotionParameters& params) = 0;
  //   virtual void step(const seconds_dbl& time) = 0;
  // };

  // /**
  //  * @class SimpleMotion
  //  * @brief
  //  */
  // class SimpleMotion : public virtual Kinematics { // impl
  // public:
  //   SimpleMotion(const MotionParameters& params = {0.0, 0.0, 0.0});

  //   const MotionParameters& state() const override
  //     { return params_; }
  //   void set(const MotionParameters& params) override;
  //   void step(const seconds_dbl& time) override;

  // private:
  //   MotionParameters params_;
  // };

  // /**
  //  * @class Oscillator
  //  * @brief
  //  */
  // class Oscillator : public virtual Kinematics { // pure interface
  // public:
  //   virtual ~Oscillator() = default;

  //   virtual double module() const = 0;
  //   virtual void remodule(double m) = 0;
  // };

  // /**
  //  * @class SimpleOscillator
  //  * @brief
  //  */
  // class SimpleOscillator : public virtual Oscillator, public SimpleMotion { // impl
  // public:
  //   SimpleOscillator(const MotionParameters& params = {0.0, 0.0, 0.0}, double module = 1.0);

  //   // Kinematics
  //   void set(const MotionParameters& params) override;
  //   void step(const seconds_dbl& time) override;

  //   // Oscillator
  //   double module() const override
  //     { return m_; }
  //   void remodule(double m) override;

  // private:
  //   double m_;
  // };

  // /**
  //  * @class SyncOscillator
  //  * @brief Synchronizable oscillator
  //  */
  // class SyncOscillator : public virtual Oscillator { // pure interface
  // public:
  //   virtual void syncPosition(double deviation, const seconds_dbl& time) = 0;
  //   virtual void syncVelocity(double deviation, const seconds_dbl& time) = 0;
  //   virtual bool isSyncingPosition() const = 0;
  //   virtual bool isSyncingVelocity() const = 0;
  // };

  // /**
  //  * @class SimpleSyncOscillator
  //  * @brief
  //  */
  // class SimpleSyncOscillator : public virtual SyncOscillator, public SimpleOscillator { // impl
  // public:
  //   SimpleSyncOscillator(const MotionParameters& params = {0.0, 0.0, 0.0}, double module = 1.0);

  //   // Kinematics
  //   void step(const seconds_dbl& time) override;

  //   // SyncOscillator
  //   void syncPosition(double deviation, const seconds_dbl& time) override;
  //   void syncVelocity(double deviation, const seconds_dbl& time) override;
  //   bool isSyncingPosition() const override;
  //   bool isSyncingVelocity() const override;

  // private:
  //   MotionParameters p_params_{0.0, 0.0, 0.0};
  //   double p_m_;
  //   double p_m_t1_;
  //   seconds_dbl p_time_{0s};

  //   // sync velocity
  //   double v_a_{0.0};
  //   seconds_dbl v_time_{0s};
  // };





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
