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
#include <utility>
#include <cassert>

namespace physics {

  using seconds_dbl = std::chrono::duration<double>;
  using minutes_dbl = std::chrono::duration<double, std::ratio<60>>;

  template<class T>
  constexpr T kInfiniteTime {std::numeric_limits<typename T::rep>::infinity()};

  template<class T>
  constexpr T kZeroTime {T::zero()};

  /**
   * @class Force
   * @brief A force that can change linearly over time
   */
  template<class T>
  struct Force
  {
    double base {0.0};
    double slope {0.0};
  };

  template<class T>
  constexpr Force<T>& operator+=(Force<T>& lhs, const Force<T>& rhs)
  {
    lhs.base += rhs.base;
    lhs.slope += rhs.slope;
    return lhs;
  }

  template<class T>
  constexpr Force<T>& operator-=(Force<T>& lhs, const Force<T>& rhs)
  {
    lhs.base -= rhs.base;
    lhs.slope -= rhs.slope;
    return lhs;
  }

  template<class T>
  constexpr Force<T> operator+(Force<T> lhs, const Force<T>& rhs)
  {
    lhs += rhs;
    return lhs;
  }

  template<class T>
  constexpr Force<T> operator-(Force<T> lhs, const Force<T>& rhs)
  {
    lhs -= rhs;
    return lhs;
  }

  template<class T>
  constexpr void applyForce(double& p, double& v, const Force<T>& f, const T& time)
  {
    double t1 = time.count();
    double t2 = t1 * t1;
    double t3 = t2 * t1;

    p += f.slope / 6.0 * t3 + f.base / 2.0 * t2 + v * t1;
    v += f.slope / 2.0 * t2 + f.base * t1;
  }

  template<class T>
  constexpr void shiftForce(Force<T>& f, const T& time)
  { f.base += f.slope * time.count(); }

  /**
   * @class Oscillator
   * @brief Simulate a rotating point mass under the influence of a force
   */
  template<class T = seconds_dbl>
  class Oscillator {
  public:
    explicit Oscillator(double module = 1.0) : m_{module} {}

    double position() const
      { return p_; }
    double velocity() const
      { return v_; }

    void resetPosition(double p = 0.0)
      { p_ = aux::math::modulo(p, m_); }
    void resetVelocity(double v = 0.0)
      { v_ = v; }
    void reset(double p = 0.0, double v = 0.0)
      {
        resetPosition(p);
        resetVelocity(v);
      }
    double module() const
      { return m_; }

    void remodule(double m)
      {
        assert(m != 0.0);
        m_ = m;
        p_ = aux::math::modulo(p_,m_);
      }

    const Force<T>& force() const
      { return f_; }

    const T& remainingForceTime() const
      { return f_time_; }

    void resetForce(const Force<T>& f = {0.0, 0.0}, const T& time = kZeroTime<T>)
      { f_ = f; f_time_ = time; }

    void resetForce(const std::pair<Force<T>, T>& f)
      { f_ = f.first; f_time_ = f.second; }

    T step(const T& time)
      {
        if (time == kZeroTime<T>)
          return kZeroTime<T>;

        T step_time = kZeroTime<T>;

        if (f_time_ > kZeroTime<T>)
        {
          step_time = std::min(f_time_, time);
          applyForce(p_,v_,f_,step_time);
          f_time_ -= step_time;
          if (f_time_ <= kZeroTime<T>)
            resetForce();
          else
            shiftForce(f_,step_time);
        }
        else
        {
          step_time = time;
          applyForce(p_,v_,{0.0, 0.0},step_time);
        }
        p_ = aux::math::modulo(p_,m_);

        return time - step_time;
      }

  private:
    double m_;
    double p_{0.0};
    double v_{0.0};
    Force<T> f_{0.0, 0.0};
    T f_time_{0.0};
  };

  template<class T>
  std::pair<Force<T>, T> computeAccelForce(double v_dev, double a)
  {
    std::pair<Force<T>, T> r {{0.0, 0.0}, kZeroTime<T>};
    auto& [r_force, r_time] = r;

    if (v_dev != 0.0)
    {
      if (a == 0.0)
      {
        r_time = kInfiniteTime<T>;
        r_force = {0.0, 0.0};
      }
      else if (T t { v_dev / a }; t > kZeroTime<T>)
      {
        r_time = t;
        r_force = {a, 0.0};
      }
      else
      {
        r_time = -t;
        r_force = {-a, 0.0};
      }
    }
    else
    {
      r_time = kZeroTime<T>;
      r_force = {a, 0.0};
    }
    return r;
  }

  template<class T>
  std::pair<Force<T>, T> computeAccelForce(double v_dev, const T& time)
  {
    std::pair<Force<T>, T> r {{0.0, 0.0}, time};
    auto& [r_force, r_time] = r;

    if (time != kZeroTime<T>)
    {
      r_force.base = v_dev / time.count();
    }
    return r;
  }

  template<class T>
  std::pair<Force<T>, T> computeSyncForce(double p_dev, double v_dev, const T& time)
  {
    std::pair<Force<T>, T> r {{0.0, 0.0}, time};
    auto& [r_force, r_time] = r;

    if (time != kZeroTime<T>)
    {
      double sync_time = time.count();
      double sync_time_squared = sync_time * sync_time;
      double sync_time_cubed = sync_time_squared * sync_time;

      if (p_dev != 0.0)
      {
        r_force.base += 6.0 * p_dev / sync_time_squared;
        r_force.slope += -12.0 * p_dev / sync_time_cubed;
      }

      if (v_dev != 0.0)
      {
        r_force.base += -2.0 * v_dev / sync_time;
        r_force.slope += 6.0 * v_dev / sync_time_squared;
      }
    }
    return r;
  }

  /**
   * @class BeatKinematics
   */
  class BeatKinematics {
  public:

    using Time = minutes_dbl;

    /**
     */
    void reset();

    /**
     * @brief Sets the number of beats of the oscillator
     *
     * The current position will be recomputed to fit the new module
     * (see @ref Oscillator::remodule).
     */
    void setBeats(double beats, bool turnover = false);

    /**
     * @brief Set the current tempo of the oscillation
     *
     * This function stops a possibly ongoing acceleration or synchronization process
     * that was previously initiated by a call to @ref accelerate or @ref synchronize
     * and resets the tempo to the given value.
     *
     * @param tempo  The new tempo in BPM
     */
    void setTempo(double tempo);

    /**
     * @brief Set up an acceleration towards a target tempo
     *
     * If the current tempo differs from the target tempo the oscillator starts
     * to accelerate towards the target tempo with the given acceleration.
     *
     * The parameter accel is the magnitude (i.e. the absolute value) of the
     * acceleration in BPM per minute. The actual signed acceleration can then
     * be accessed by a call to @ref acceleration().
     *
     * @param accel  The magnitude of acceleration in BPM per minute
     * @param target  The target tempo in BPM
     */
    void accelerate(double accel, double target);

    /**
     * This function applies a force to synchronize the underlying oscillator (source)
     * with another oscillator (target) over a specified time. The parameters beat_dev
     * and tempo_dev are the beat and tempo deviations of the (unsynced) source oscillator
     * from the target oscillator after the sync time.
     * If one of the deviation parameters is zero, no further computation is necessary
     * on the client side and the other parameter is just the desired deviation after
     * the synchronization process.
     * On the other hand, if both beat position and tempo are to be changed the client
     * should compute the difference in beat position and tempo after sync time. In the
     * simple case of two oscillators to be exactly synchronized in tempo and position
     * that would be:
     *
     * tempo_dev = tempo_tgt - tempo_src
     * beat_dev = (tempo_tgt * sync_time + pos_tgt) - (tempo_src * sync_time + pos_src)
     *
     * where tempo_src, pos_src and tempo_tgt, pos_tgt are the velocities and position
     * of the source and target oscillators, respectively.
     */
    void synchronize(double beat_dev, double tempo_dev, const Time& time);

    /** Stop an ongoing acceleration process */
    void stopAcceleration();

    /** Stop an ongoing synchronization process */
    void stopSynchronization();

    bool isAccelerating() const
      { return (force_mode_ == ForceMode::kAccelForce); }
    bool isSynchronizing() const
      { return (force_mode_ == ForceMode::kSyncForce); }

    double position() const
      { return osc_.position(); }
    double tempo() const
      { return osc_.velocity(); }
    double acceleration() const
      { return osc_.force().base; }

    /**
     */
    void step(Time time);

    /**
     */
    Time arrival(double p_dev) const;

  private:
    Oscillator<Time> osc_;

    double tempo_{0.0};     // beats / min
    double target_{0.0};    // beats / min
    double accel_{0.0};     // beats / min²
    double sync_beat_dev_{0.0};
    double sync_tempo_dev_{0.0};
    double sync_start_tempo_{0.0};
    Time sync_time_{0.0};

    enum class ForceMode
    {
      kNoForce = 0,
      kAccelForce =1,
      kSyncForce = 2
    };

    ForceMode force_mode_{ForceMode::kNoForce};

    void switchForceMode(ForceMode mode);
    void updateOscForce(ForceMode mode);
  };

  /**
   * @class PendulumKinematics
   */
  class PendulumKinematics {
  public:
    // ctor
    PendulumKinematics() = default;

    void reset(double theta = 0.0, double omega = 0.0)
      {
        osc_.reset(theta, omega);
        osc_.resetForce();
      }

    double theta() const
      { return osc_.position(); }
    double omega() const
      { return osc_.velocity(); }
    double alpha() const
      { return osc_.force().base; }

    void shutdown(const seconds_dbl& time);

    /** See comments for BeatKinematics::synchronize. */
    void synchronize(double theta_dev, double omega_dev, const seconds_dbl& time);

    void step(seconds_dbl time);

  private:
    Oscillator<seconds_dbl> osc_{2.0 * M_PI};
  };

}//namespace physics
#endif//GMetronome_Physics_h
