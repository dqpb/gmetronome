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

#ifndef GMetronome_ProfileListStore_h
#define GMetronome_ProfileListStore_h

#include <gtkmm.h>
#include <iostream>

class ProfileListStore : public Gtk::ListStore
{
protected:
  ProfileListStore();

public:
  //Tree model columns
  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:

    ModelColumns()
    {
      add(id_);
      add(title_);
      add(description_);
      add(draggable_);
      add(receivesdrags_);
    }

    Gtk::TreeModelColumn<Glib::ustring> id_;
    Gtk::TreeModelColumn<Glib::ustring> title_;
    Gtk::TreeModelColumn<Glib::ustring> description_;

    Gtk::TreeModelColumn<bool> draggable_;
    Gtk::TreeModelColumn<bool> receivesdrags_;
  };

  ModelColumns columns_;

  static Glib::RefPtr<ProfileListStore> create();
};

#endif//GMetronome_ProfileListStore_h
