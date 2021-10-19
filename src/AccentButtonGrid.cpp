/*
 * Copyright (C) 2020 The GMetronome Team
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

AccentButtonGrid::AccentButtonGrid(std::size_t size, std::size_t grouping)
{
  set_has_window(false);
  set_redraw_on_allocate(false);
  
  resizeButtonsVector(size);

  setGrouping(grouping);
}

AccentButtonGrid::~AccentButtonGrid()
{
  resizeButtonsVector(0);  
}

void AccentButtonGrid::onAccentChanged(std::size_t index)
{
  signal_accent_changed_.emit(index);
}

bool AccentButtonGrid::resizeButtonsVector(std::size_t new_size)
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

  if (buttons_.size() != old_size)
    return true;
  else
    return false;
}

bool AccentButtonGrid::setGrouping(std::size_t grouping)
{
  if (grouping < 1)
    grouping = 1;

  bool ret = ( grouping != grouping_ );
  
  grouping_ = grouping;

  return ret;
}

void AccentButtonGrid::resize(std::size_t new_size)
{
  if ( resizeButtonsVector(new_size) )
    queue_resize();
}

void AccentButtonGrid::regroup(std::size_t grouping)
{
  if ( setGrouping(grouping) )
    queue_resize();
}

Gtk::SizeRequestMode AccentButtonGrid::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

void AccentButtonGrid::updateCellDimensions() const
{
  int result_width_min = 1;
  int result_height_min = 1;
  
  int result_width_nat = 1;
  int result_height_nat = 1;
  
  int button_min;
  int button_nat;
  
  for ( auto& button : buttons_ )
  {
    button->get_preferred_width(button_min, button_nat);
    result_width_min = std::max(result_width_min, button_min);
    result_width_nat = std::max(result_width_nat, button_nat);
    
    button->get_preferred_height(button_min, button_nat);
    result_height_min = std::max(result_height_min, button_min);
    result_height_nat = std::max(result_height_nat, button_nat);
  }
  
  cell_width_min_ = result_width_min;
  cell_height_min_ = result_height_min;  
  
  cell_width_nat_ = result_width_nat;
  cell_height_nat_ = result_height_nat;  

  std::size_t min_cells_per_row = 
    std::max( (std::size_t) 1, std::min(buttons_.size(), grouping_) );
  
  group_width_min_ = min_cells_per_row * cell_width_min_;
  group_width_nat_ = min_cells_per_row * cell_width_nat_;
}

void AccentButtonGrid::numRowsForWidth(int width,
                                       int& num_rows_min,
                                       int& num_rows_nat) const
{
  std::size_t num_groups = std::ceil( (double) buttons_.size() / grouping_ );
  std::size_t max_groups_per_row_min = ( width / group_width_min_ );
  std::size_t max_groups_per_row_nat = ( width / group_width_nat_ );
  
  if ( max_groups_per_row_min < 1 )
    max_groups_per_row_min = 1;

  if ( max_groups_per_row_nat < 1 )
    max_groups_per_row_nat = 1;
  
  num_rows_min = std::ceil( (double) num_groups / max_groups_per_row_min );
  num_rows_nat = std::ceil( (double) num_groups / max_groups_per_row_nat );

  //int groups_per_row = std::ceil( (double) num_groups / num_rows);
  //int max_columns = groups_per_row * subdiv;  
}

void AccentButtonGrid::numGroupsPerRowForHeight(int height,
                                                int& groups_per_row_min,
                                                int& groups_per_row_nat) const
{
  std::size_t num_groups = std::ceil( (double) buttons_.size() / grouping_ );

  std::size_t num_rows_min = ( height / cell_height_min_ );
  std::size_t num_rows_nat = ( height / cell_height_nat_ );
  
  if ( num_rows_min < 1 )
    num_rows_min = 1;
  
  if ( num_rows_nat < 1 )
    num_rows_nat = 1;
  
  groups_per_row_min = std::ceil( (double) num_groups / num_rows_min );
  groups_per_row_nat = std::ceil( (double) num_groups / num_rows_nat );
}

void AccentButtonGrid::get_preferred_width_vfunc(int& minimum_width,
                                                 int& natural_width) const
{
  updateCellDimensions();
  
  minimum_width = group_width_min_;
  natural_width = group_width_nat_;  
}

void AccentButtonGrid::get_preferred_height_for_width_vfunc(int width,
                                                            int& minimum_height,
                                                            int& natural_height) const
{
  updateCellDimensions();

  int num_rows_min;
  int num_rows_nat;

  numRowsForWidth( width, num_rows_min, num_rows_nat );
  
  minimum_height = num_rows_min * cell_height_min_;
  natural_height = num_rows_nat * cell_height_nat_;
}

void AccentButtonGrid::get_preferred_height_vfunc(int& minimum_height,
                                                  int& natural_height) const
{
  updateCellDimensions();

  minimum_height = cell_height_min_;
  natural_height = cell_height_nat_;
}

void AccentButtonGrid::get_preferred_width_for_height_vfunc(int height,
                                                            int& minimum_width,
                                                            int& natural_width) const
{
  updateCellDimensions();

  int groups_per_row_min;
  int groups_per_row_nat;

  numGroupsPerRowForHeight(height, groups_per_row_min, groups_per_row_nat);
  
  minimum_width = groups_per_row_min * group_width_min_;
  natural_width = groups_per_row_nat * group_width_nat_;
}

void AccentButtonGrid::on_size_allocate(Gtk::Allocation& alloc)
{
  updateCellDimensions();

  std::size_t num_groups = std::ceil( (double) buttons_.size() / grouping_ );

  int num_rows_min;
  int num_rows_nat;
  
  numRowsForWidth( alloc.get_width(), num_rows_min, num_rows_nat );

  if ( num_rows_nat > 0 )
  {  
    //int groups_per_row_min = std::ceil( (double) num_groups / num_rows_min );
    int groups_per_row_nat = std::ceil( (double) num_groups / num_rows_nat );
    
    int cells_per_row_nat = groups_per_row_nat * grouping_;

    int left_offset_nat = ( alloc.get_width() - ( cells_per_row_nat * cell_width_nat_ ) ) / 2;
    int top_offset_nat = ( alloc.get_height() - ( num_rows_nat * cell_height_nat_ ) ) / 2;

    Gtk::Allocation button_alloc (0, 0, cell_width_nat_, cell_height_nat_);
    
    for ( std::size_t index = 0; index < buttons_.size(); ++index )
    {
      button_alloc.set_x( alloc.get_x()
                          + left_offset_nat
                          + ( index % cells_per_row_nat ) * cell_width_nat_ );
      
      button_alloc.set_y( alloc.get_y()
                          + top_offset_nat
                          + ( index / cells_per_row_nat ) * cell_height_nat_ );
      

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
