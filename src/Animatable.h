/*
 * Copyright (C) 2025 The GMetronome Team
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

#ifndef GMetronome_Animatable_h
#define GMetronome_Animatable_h

#include <glibmm/refptr.h>
#include <gdkmm/frameclock.h>
#include <chrono>

// CRTP to be used with Gtk::Widget types that need exact frame timing
// to display continuous animations.
template<typename T>
class Animatable {
public:
  void startAnimation()
    {
      if (animation_running_)
        return;

      static_cast<T*>(this)->add_tick_callback(
        sigc::mem_fun(*this, &Animatable<T>::updateAnimation_) );

      animation_running_ = true;
    }
  void stopAnimation()
    { animation_running_ = false; }

  bool isAnimationRunning() const
    { return animation_running_; }

  virtual void updateAnimation(const Glib::RefPtr<Gdk::FrameClock>&) = 0;

  static std::chrono::microseconds
  getFrameTime(const Glib::RefPtr<Gdk::FrameClock>& clock)
    {
      using std::chrono::microseconds;

      microseconds frame_time {0};
      if (clock)
      {
        auto timings = clock->get_current_timings();
        if (timings)
        {
          frame_time = microseconds(timings->get_predicted_presentation_time());

          if (frame_time.count() == 0)
            frame_time = microseconds(timings->get_presentation_time());
        }
        // no timings or (predicted) presentation time available
        if (frame_time.count() == 0)
          frame_time = microseconds(clock->get_frame_time());
      }
      return frame_time;
    }

private:
  bool animation_running_{false};

  bool updateAnimation_(const Glib::RefPtr<Gdk::FrameClock>& clock)
    {
      if (animation_running_) updateAnimation(clock);
      return animation_running_;
    }
};
#endif//GMetronome_Animatable_h
