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

#ifndef GMetronome_StateButton_h
#define GMetronome_StateButton_h

#include <sigc++/sigc++.h>
#include <gtkmm.h>

#include <utility>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>

template<typename S>
class StateButton : public Gtk::Button {
public:
  using State = S;
  using ConfigEntry = std::pair<State, std::string>; // state and icon name
  using Configuration = std::vector<ConfigEntry>;

public:
  StateButton(Configuration config);

  const State& state() const;
  void setState(const State& state);

  sigc::signal<void(const State&)> signalStateChanged()
    { return signal_state_changed_; }

private:
  const Configuration config_;
  std::size_t current_index_{0};
  sigc::signal<void(const State&)> signal_state_changed_;

  void next(bool cycle = true);
  void prev(bool cycle = true);

  void on_clicked() override;
  bool on_scroll_event(GdkEventScroll *scroll_event) override;
};

template<typename T>
StateButton<T>::StateButton(Configuration config)
  : config_{std::move(config)}
{
  assert(!config_.empty());
  set_image_from_icon_name(config_[current_index_].second);
  add_events(Gdk::SCROLL_MASK);
}

template<typename T>
auto StateButton<T>::state() const -> const State&
{
  return config_[current_index_].first;
}

template<typename T>
void StateButton<T>::setState(const State& new_state)
{
  auto it = std::find_if(config_.begin(), config_.end(),
                         [&new_state] (const auto& c) { return new_state == c.first; });

  if (it != config_.end())
  {
    std::size_t new_index = std::distance(config_.begin(), it);
    assert(new_index < config_.size());
    if (static_cast<std::size_t>(new_index) != current_index_)
    {
      current_index_ = new_index;
      set_image_from_icon_name(config_[current_index_].second);
      signal_state_changed_.emit(state());
    }
  }
}

template<typename T>
void StateButton<T>::next(bool cycle)
{
  if (current_index_ != (config_.size() - 1))
    current_index_ += 1;
  else if (cycle)
    current_index_ = 0;
  else
    return;

  set_image_from_icon_name(config_[current_index_].second);
  signal_state_changed_.emit(state());
}

template<typename T>
void StateButton<T>::prev(bool cycle)
{
  if (current_index_ != 0)
    current_index_ -= 1;
  else if (cycle)
    current_index_ = config_.size() - 1;
  else
    return;

  set_image_from_icon_name(config_[current_index_].second);
  signal_state_changed_.emit(state());
}

template<typename T>
void StateButton<T>::on_clicked()
{
  next(true);
  Button::on_clicked();
}

template<typename T>
bool StateButton<T>::on_scroll_event(GdkEventScroll *scroll_event)
{
  switch (scroll_event->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_RIGHT:
    next(false);
    break;

  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_LEFT:
    prev(false);
    break;

  default:
    break;
  };

  return Gtk::Button::on_scroll_event(scroll_event);
}

#endif//GMetronome_StateButton_h
