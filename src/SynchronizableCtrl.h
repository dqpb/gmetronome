/*
 * Copyright (C) 2026 The GMetronome Team
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

#ifndef GMetronome_SynchronizableCtrl_h
#define GMetronome_SynchronizableCtrl_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ticker.h"
#include "Synchronizable.h"

#include <glibmm.h>
#include <chrono>
#include <list>
#include <optional>

#ifndef NDEBUG
# include <iostream>
#endif

/**
 * @class TickerInfoQueue
 */
class TickerInfoQueue {
public:
  /** Append a Ticker::Info object. */
  void push(const audio::Ticker::Info& info);

  /**
   *  Get next Ticker::Info, i.e. the next object in the queue where the sum
   *  of the timestamp and backend latency is less or equal to the time parameter
   *  (or an empty std::optional if no such object exists).
   */
  std::optional<audio::Ticker::Info> pop(const std::chrono::microseconds& time);

  /** Remove all objects in the queue. */
  void clear();

private:
  using ContainerType = std::list<audio::Ticker::Info>;
  ContainerType q_;
  ContainerType::const_iterator head_it_{q_.end()}; //!< head of the queue

  void forwardHeadIterator(const std::chrono::microseconds& time);
};

/**
 * @class GlibTimer
 */
class GlibTimer {
public:
  GlibTimer(const std::chrono::milliseconds& time)
    {
      c_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GlibTimer::hdl), time.count());
    }
  ~GlibTimer()
    { c_.disconnect(); }
  sigc::signal<void> signalTimeout()
    { return s_; }

private:
  sigc::signal<void> s_;
  sigc::connection c_;

  bool hdl() { s_.emit(); return true; }
};

/**
 * @class SynchronizableCtrl
 */
template<typename TimerT>
class SynchronizableCtrl {
public:
  ~SynchronizableCtrl()
    { if (isRunning()) stop(); }

  void registerSynchronizable(Synchronizable* s)
    {
      if (std::find(syncs_.begin(), syncs_.end(), s) == syncs_.end())
      {
        syncs_.push_back(s);
        if (isRunning()) s->startSynchronization();
      }
#ifndef NDEBUG
      else {
        std::cerr << "SynchronizableCtrl: Warning: Synchronizable already registered" << std::endl;
      }
#endif
    }
  void unregisterSynchronizable(Synchronizable* s)
    {
      if (auto it = std::find(syncs_.begin(), syncs_.end(), s); it != syncs_.end())
      {
        if (isRunning()) (*it)->stopSynchronization();
        syncs_.erase(it);
      }
#ifndef NDEBUG
      else {
        std::cerr << "SynchronizableCtrl: Warning: Synchronizable not registered" << std::endl;
      }
#endif
    }

  bool isRegistered(const Synchronizable* s)
    { return std::find(syncs_.begin(), syncs_.end(), s) != syncs_.end(); }

  void enrollTickerInfo(const audio::Ticker::Info& info)
    {
      if (isRunning())
      {
        info_q_.push(info);
      }
#ifndef NDEBUG
      else {
        std::cerr << "SynchronizableCtrl: failed to enroll Ticker::Info in stopped controller"
                  << std::endl;
      }
#endif
    }

  void setSynchronization(const std::chrono::milliseconds& time)
    { sync_time_ = time; }

  void start()
    {
      if (isRunning()) stop();
      for (auto s : syncs_) s->startSynchronization();
      startTimer();
    }

  void stop()
    {
      if (isRunning())
      {
        stopTimer();
        info_q_.clear();
        for (auto s : syncs_) s->stopSynchronization();
      }
    }

  bool isRunning() const
    { return (bool) timer_; }

private:
  // determines video sync accuracy (maybe gsettings candidate?)
  static constexpr std::chrono::milliseconds kTimerResolution{20};

  TickerInfoQueue info_q_;
  std::unique_ptr<TimerT> timer_;
  std::vector<Synchronizable*> syncs_;
  std::chrono::microseconds sync_time_{0};

  void startTimer()
    {
      timer_ = std::make_unique<TimerT>(kTimerResolution);
      timer_->signalTimeout().connect(sigc::mem_fun(*this, &SynchronizableCtrl::step));
    }

  void stopTimer()
    { timer_.reset(); }

  void step()
    {
      std::chrono::microseconds time {g_get_monotonic_time()};

      if (auto info = info_q_.pop(time - sync_time_ + (kTimerResolution / 2)); info.has_value())
      {
        for (auto& s : syncs_)
          s->synchronize(info.value(), sync_time_);
      }
    }
};

#endif//GMetronome_SynchronizableCtrl_h
