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

  constexpr seconds_dbl kInfiniteTime {std::numeric_limits<double>::infinity()};
  constexpr seconds_dbl kZeroTime {seconds_dbl::zero()};

  /**
   * @class Force
   * @brief A force that can change linearly over time
   */
  struct Force
  {
    double base {0.0};
    double slope {0.0};
  };

  constexpr Force& operator+=(Force& lhs, const Force& rhs)
  {
    lhs.base += rhs.base;
    lhs.slope += rhs.slope;
    return lhs;
  }

  constexpr Force& operator-=(Force& lhs, const Force& rhs)
  {
    lhs.base -= rhs.base;
    lhs.slope -= rhs.slope;
    return lhs;
  }

  constexpr Force operator+(Force lhs, const Force& rhs)
  {
    lhs += rhs;
    return lhs;
  }
  constexpr Force operator-(Force lhs, const Force& rhs)
  {
    lhs -= rhs;
    return lhs;
  }

  constexpr
  void applyForce(double& p, double& v, const Force& f, const seconds_dbl& time)
  {
    double t1 = time.count();
    double t2 = t1 * t1;
    double t3 = t2 * t1;

    p += f.slope / 6.0 * t3 + f.base / 2.0 * t2 + v * t1;
    v += f.slope / 2.0 * t2 + f.base * t1;
  }

  constexpr
  void shiftForce(Force& f, const seconds_dbl& time)
  { f.base += f.slope * time.count(); }

  /**
   * @class Oscillator
   * @brief Simulate a rotating point mass under the influence of a force
   */
  class Oscillator {
  public:
    explicit Oscillator(double module = 1.0) : m_{module} {}

    double position() const
      { return p_; }
    double velocity() const
      { return v_; }

    void resetPosition(double p = 0.0)
      { p_ = p; }
    void resetVelocity(double v = 0.0)
      { v_ = v; }
    void reset(double p = 0.0, double v = 0.0)
      { p_ = p; v_ = v; }

    double module() const
      { return m_; }

    void remodule(double m)
      {
        assert(m != 0.0);
        m_ = m;
        p_ = aux::math::modulo(p_,m_);
      }

    const Force& force() const
      { return f_; }

    void resetForce(const Force& f = {0.0, 0.0})
      { f_ = f; }

    void step(const seconds_dbl& time)
      {
        applyForce(p_,v_,f_,time);
        p_ = aux::math::modulo(p_,m_);
        shiftForce(f_,time);
      }

  private:
    double m_;
    double p_{0.0};
    double v_{0.0};
    Force f_{0.0, 0.0};
  };

  /**
   * @class BeatKinematics
   */
  class BeatKinematics {
  public:
    // ctor
    explicit BeatKinematics(double beats = 4.0) : osc_(beats) {}

    /**
     * @function reset
     */
    void reset();

    /**
     * @function setBeats
     */
    void setBeats(double beats)
      { osc_.remodule(beats); }

    /**
     * @function setTempo
     * @brief Set the current tempo of the oscillation
     *
     * Setting the current tempo does not affect the target tempo and
     * the target acceleration, i.e. the oscillator will continue to
     * change the speed towards the target tempo.
     */
    void setTempo(double tempo);

    /**
     * @function setTargetTempo
     * @brief Sets the target tempo of the oscillation
     */
    void setTargetTempo(double tempo);

    /**
     * @function setAcceleration
     * @brief Sets the acceleration used to reach the target tempo
     */
    void setAcceleration(double accel);

    /**
     * @function synchronize
     */
    void synchronize(double beat_dev, double tempo_dev, const seconds_dbl& time);

    double position() const;
    double tempo() const;
    double acceleration() const;

    /**
     * @function step
     */
    void step(seconds_dbl time);

    /**
     * @function arrival
     */
    seconds_dbl arrival(double p_dev) const;

  private:
    Oscillator osc_{4.0};

    double tempo_{0.0};           // beats / s
    double target_tempo_{0.0};    // beats / s
    double accel_{0.0};           // 1.0 / s²
    double sync_beat_dev_{0.0};
    double sync_tempo_dev_{0.0};
    double sync_start_tempo_{0.0};
    seconds_dbl sync_time_{0.0};

    enum class ForceMode
    {
      kNoForce = 0,
      kAccelForce =1,
      kSyncForce = 2
    };

    ForceMode force_mode_{ForceMode::kNoForce};

    seconds_dbl accel_r_time_{0.0};
    seconds_dbl sync_r_time_{0.0};

    void switchForceMode(ForceMode mode);
    void updateOscForce(ForceMode mode);

    static
    std::pair<Force, seconds_dbl>
    computeAccelForce(double v_dev, double a);

    static
    std::pair<Force, seconds_dbl>
    computeSyncForce(double p_dev, double v_dev, const seconds_dbl& time);
  };

}//namespace physics
#endif//GMetronome_Physics_h
