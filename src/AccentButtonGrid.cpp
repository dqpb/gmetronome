/*
 * Copyright (C) 2020-2023 The GMetronome Team
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

#include "AccentButtonGrid.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <cassert>

namespace {

  using std::chrono::microseconds;
  using std::chrono::milliseconds;

  using std::literals::chrono_literals::operator""us;
  using std::literals::chrono_literals::operator""ms;

  constexpr microseconds kAnimationScheduleTimeFrame = 350ms;

}//unnamed namespace

AccentButtonGrid::AccentButtonGrid()
{
  set_has_window(false);
  set_redraw_on_allocate(false);

  updateAccentButtons(meter_);
}

AccentButtonGrid::~AccentButtonGrid()
{
  resizeButtonsVector(0);
}

void AccentButtonGrid::setMeter(const Meter& meter)
{
  if (meter.division() != meter_.division() ||
      meter.beats() < meter_.beats())
  {
    cancelButtonAnimations();
  }
  updateAccentButtons(meter);
  meter_ = meter;
}

void AccentButtonGrid::start()
{ }

void AccentButtonGrid::stop()
{
  cancelButtonAnimations();
}

void AccentButtonGrid::synchronize(const audio::Ticker::Statistics& stats,
                                   const std::chrono::microseconds& sync)
{
  const int beats = meter_.beats();
  const int division = meter_.division();
  const int n_accents = beats * division;

  if (stats.beats == beats && stats.division == division)
  {
    int next_accent = (stats.accent + 1) % n_accents;
    assert(next_accent >= 0);

    if (static_cast<std::size_t>(next_accent) < buttons_.size())
    {
      microseconds time = stats.timestamp
        + stats.backend_latency
        + stats.next_accent_delay
        + sync;

      microseconds now {g_get_monotonic_time()};

      if ( (time - now) < kAnimationScheduleTimeFrame)
        buttons_[next_accent]->scheduleAnimation(time.count());
    }
  }
}

void AccentButtonGrid::cancelButtonAnimations()
{
  for (auto& button : buttons_)
    button->cancelAnimation();
}

void AccentButtonGrid::updateAccentButtons(const Meter& meter)
{
  const auto& new_accents = meter.accents();

  std::size_t new_size = new_accents.size();
  std::size_t old_size = buttons_.size();

  int old_division = meter_.division();
  int new_division = meter.division();

  bool need_relabel = false;

  if (new_size > old_size || new_division != old_division)
    need_relabel = true;

  if (new_size != old_size)
  {
    resizeButtonsVector(new_size);
    queue_resize();
  }

  if (need_relabel)
  {
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
      Glib::ustring label = ( index % new_division == 0 ) ?
        Glib::ustring::format( index / new_division + 1 ) : "";

      buttons_[index]->setLabel(label);
      buttons_[index]->setAccentState( new_accents[index] );
    }
  }
  else
  {
    for (std::size_t index = 0; index < buttons_.size(); ++index)
      buttons_[index]->setAccentState( new_accents[index] );
  }
}

void AccentButtonGrid::resizeButtonsVector(std::size_t new_size)
{
  std::size_t old_size = buttons_.size();

  for ( std::size_t index = new_size; index < old_size; ++index )
  {
    buttons_[index]->hide();
    buttons_[index]->unparent();
    delete buttons_[index];
  }

  buttons_.resize(new_size);

  for ( std::size_t index = old_size; index < new_size; ++index )
  {
    buttons_[index] = new AccentButton();
    buttons_[index]->set_parent(*this);
    buttons_[index]->show();

    buttons_[index]->signal_accent_state_changed()
      .connect(sigc::bind( sigc::mem_fun(*this, &AccentButtonGrid::onAccentChanged), index));
  }
}

void AccentButtonGrid::onAccentChanged(std::size_t index)
{
  meter_.setAccent(index, buttons_[index]->getAccentState());
  signal_accent_changed_.emit(index);
}

Gtk::SizeRequestMode AccentButtonGrid::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

void AccentButtonGrid::updateCellDimensions() const
{
  cell_width_ = 1;
  cell_height_ = 1;

  if (!buttons_.empty())
  {
    // The accent button size should be equal for all buttons, so the cell size
    // is just the natural button size of one of the buttons, e.g. the first one.
    int min, nat;

    buttons_.front()->get_preferred_width(min, nat);
    cell_width_ = nat;

    buttons_.front()->get_preferred_height(min, nat);
    cell_height_ = nat;
  }

  std::size_t group_size = meter_.division();

  std::size_t min_cells_per_row =
    std::max<std::size_t>(1, std::min<std::size_t>(buttons_.size(), group_size));

  group_width_ = min_cells_per_row * cell_width_;
}

int AccentButtonGrid::numRowsForWidth(int width) const
{
  std::size_t group_size = meter_.division();
  std::size_t num_groups = meter_.beats();

  std::size_t max_groups_per_row = (width / group_width_);
  max_groups_per_row = std::min(max_groups_per_row, kMaxButtonsPerRow / group_size);

  if ( max_groups_per_row < 1 )
    max_groups_per_row = 1;

  return std::ceil((double) num_groups / max_groups_per_row);
}

int AccentButtonGrid::numGroupsPerRowForHeight(int height) const
{
  std::size_t num_groups = meter_.beats();
  std::size_t num_rows = std::max(1, (height / cell_height_));

  return std::ceil((double) num_groups / num_rows);
}

void AccentButtonGrid::get_preferred_width_vfunc(int& minimum_width,
                                                 int& natural_width) const
{
  updateCellDimensions();

  natural_width = std::max(group_width_, kMaxButtonsPerRow * cell_width_);
  minimum_width = natural_width;
}

void AccentButtonGrid::get_preferred_height_for_width_vfunc(int width,
                                                            int& minimum_height,
                                                            int& natural_height) const
{
  natural_height = numRowsForWidth(width) * cell_height_;
  minimum_height = natural_height;
}

void AccentButtonGrid::get_preferred_height_vfunc(int& minimum_height,
                                                  int& natural_height) const
{
  updateCellDimensions();

  natural_height = cell_height_;
  minimum_height = natural_height;
}

void AccentButtonGrid::get_preferred_width_for_height_vfunc(int height,
                                                            int& minimum_width,
                                                            int& natural_width) const
{
  natural_width = numGroupsPerRowForHeight(height) * group_width_;
  minimum_width = natural_width;
}

void AccentButtonGrid::on_size_allocate(Gtk::Allocation& alloc)
{
  std::size_t num_rows = numRowsForWidth(alloc.get_width());
  std::size_t num_groups = meter_.beats();
  std::size_t group_size = meter_.division();

  if (num_rows > 0)
  {
    int groups_per_row = std::ceil((double) num_groups / num_rows);
    int cells_per_row = groups_per_row * group_size;

    int padding_width = (alloc.get_width() - kMaxButtonsPerRow * cell_width_) / 2;
    int padding_height = 0;

    double padding_x = (padding_width > 0 && kMaxButtonsPerRow > 1) ?
      (double) padding_width / (kMaxButtonsPerRow - 1) : 0.0;

    int padding_y = 0;

    int left_offset = (alloc.get_width() - cells_per_row * (cell_width_ + padding_x)) / 2;
    int top_offset = (alloc.get_height() - (num_rows * cell_height_) - padding_height) / 2;

    Gtk::Allocation button_alloc (0, 0, cell_width_, cell_height_);

    for ( std::size_t index = 0; index < buttons_.size(); ++index )
    {
      if (Gtk::Widget::get_direction() == Gtk::TEXT_DIR_RTL)
      {
        button_alloc.set_x( alloc.get_x() + alloc.get_width()
                            - left_offset
                            - cell_width_
                            - ( index % cells_per_row ) * (cell_width_ + padding_x));
      }
      else
      {
        button_alloc.set_x( alloc.get_x()
                            + left_offset
                            + ( index % cells_per_row ) * (cell_width_  + padding_x));
      }
      button_alloc.set_y( alloc.get_y()
                          + top_offset
                          + ( index / cells_per_row ) * (cell_height_ + padding_y) );

      buttons_[index]->size_allocate(button_alloc);
    }
  }

  set_allocation(alloc);
}

void AccentButtonGrid::forall_vfunc(gboolean,
                                    GtkCallback callback,
                                    gpointer callback_data)
{
  for (auto& button : buttons_)
    callback((GtkWidget*)button->gobj(), callback_data);
}


void AccentButtonGrid::on_add(Gtk::Widget* child)
{}

void AccentButtonGrid::on_remove(Gtk::Widget* child)
{}

GType AccentButtonGrid::child_type_vfunc() const
{
  return G_TYPE_NONE;
}
