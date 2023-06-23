/*
 * Copyright (C) 2020, 2021 The GMetronome Team
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

#ifndef GMetronome_AccentButtonGrid_h
#define GMetronome_AccentButtonGrid_h

#include "AccentButton.h"

#include <gtkmm/container.h>
#include <sigc++/sigc++.h>
#include <vector>
#include <cmath>

class AccentButtonGrid : public Gtk::Container {

public:
  AccentButtonGrid(std::size_t size = 0, std::size_t grouping = 0);

  virtual ~AccentButtonGrid();

  void resize(std::size_t size);

  std::size_t size() const
    { return buttons_.size(); }

  void regroup(std::size_t grouping);

  std::size_t grouping() const
    { return grouping_; }

  std::size_t groups() const
    { return (grouping_ != 0) ? std::ceil((double) size() / grouping_) : 0; }

  const std::vector<AccentButton*>& buttons() const
    { return buttons_; }

  const AccentButton& operator[](std::size_t index) const
    { return *buttons_[index]; }

  AccentButton& operator[](std::size_t index)
    { return *buttons_[index]; }

  sigc::signal<void(std::size_t index)> signal_accent_changed()
    { return signal_accent_changed_; }

private:
  Gtk::SizeRequestMode get_request_mode_vfunc() const override;

  void get_preferred_width_vfunc(int& minimum_width,
                                 int& natural_width) const override;

  void get_preferred_height_for_width_vfunc(int width,
                                            int& minimum_height,
                                            int& natural_height) const override;

  void get_preferred_height_vfunc(int& minimum_height,
                                  int& natural_height) const override;

  void get_preferred_width_for_height_vfunc(int height,
                                            int& minimum_width,
                                            int& natural_width) const override;

  void on_size_allocate(Gtk::Allocation& allocation) override;

  void forall_vfunc(gboolean include_internals,
                    GtkCallback callback,
                    gpointer callback_data) override;

  void on_add(Gtk::Widget* child) override;

  void on_remove(Gtk::Widget* child) override;

  GType child_type_vfunc() const override;

private:
  std::vector<AccentButton*> buttons_;
  std::size_t grouping_;

  // cache
  mutable int cell_width_min_;
  mutable int cell_height_min_;
  mutable int cell_width_nat_;
  mutable int cell_height_nat_;
  mutable int group_width_min_;
  mutable int group_width_nat_;

  sigc::signal<void(std::size_t index)> signal_accent_changed_;

  void onAccentChanged(std::size_t index);
  bool resizeButtonsVector(std::size_t size);
  bool setGrouping(std::size_t grouping);
  void updateCellDimensions() const;

  void numRowsForWidth(int width,
                       int& num_rows_min,
                       int& num_rows_nat) const;

  void numGroupsPerRowForHeight(int height,
                                int& groups_per_row_min,
                                int& groups_per_row_nat) const;
};

#endif//GMetronome_AccentButtonGrid_h
