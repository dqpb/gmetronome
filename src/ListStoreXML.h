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

#ifndef GMetronome_ListStoreXML_h
#define GMetronome_ListStoreXML_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ListStore.h"
#include "Error.h"

#include <giomm.h>
#include <glibmm.h>

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <utility>
#include <array>
#include <cassert>

#ifndef NDEBUG
# include <iostream>
#endif

/**
 * @class ListStoreXMLParser
 */
template<typename T, typename I>
class ListStoreXMLParser : public Glib::Markup::Parser {
public:
  using Type = T;
  using Identifier = I;
  using EntryMap = std::map<Identifier, Type>;
  using OrderVector = std::vector<Identifier>;

  virtual EntryMap moveMap() = 0;
  virtual OrderVector moveOrder() = 0;
};

/**
 * @class ListStoreXMLWriter
 */
template<typename T, typename I>
class ListStoreXMLWriter {
public:
  using Type = T;
  using Identifier = I;

  virtual const std::string& topLevelElementName() const = 0;

  virtual void writeEntry(Glib::RefPtr<Gio::FileOutputStream> ostream,
                          const Type& entry,
                          const Identifier& id) = 0;
};

/**
 * @class ListStoreXML
 */
template<typename T, typename I, typename H, typename P, typename W>
class ListStoreXML : public ListStore<T,I,H> {

  static_assert(std::is_base_of_v<ListStoreXMLParser<T,I>, P>,
                "P must be derived from ListStoreXMLParser<T,I>");
  static_assert(std::is_base_of_v<ListStoreXMLWriter<T,I>, W>,
                "W must be derived from ListStoreXMLWriter<T,I>");
public:
  using typename ListStore<T,I,H>::Type;
  using typename ListStore<T,I,H>::Header;
  using typename ListStore<T,I,H>::Identifier;
  using typename ListStore<T,I,H>::Primer;

  using Parser = P;
  using Writer = W;

public:
  // Construction and destruction
  ListStoreXML(std::string path, std::string import_path = "")
    : path_{std::move(path)}, import_path_{std::move(import_path)}
    { /* nothing */ }
  ListStoreXML(ListStoreXML&& other) = default;
  ListStoreXML& operator=(ListStoreXML&& other) = default;
  ~ListStoreXML() override;

  // Interface
  std::vector<Primer> list() override;
  Type load(Identifier id) override;
  void store(Identifier id, const Type& entry) override;
  void reorder(const std::vector<Identifier>& order) override;
  void remove(Identifier id) override;
  void flush() override;

private:
  using EntryMap = std::map<Identifier, Type>;
  EntryMap t_map_;
  std::vector<Identifier> t_order_;

  std::string path_;
  std::string import_path_;

  bool pending_import_{true};
  bool import_error_{false};

  bool pending_export_{false};
  bool export_error_{false};

  void importData();
  void exportData();
};

template<typename T, typename I, typename H, typename P, typename W>
ListStoreXML<T,I,H,P,W>::~ListStoreXML()
{
  if (pending_export_)
  {
    try { exportData(); }
    catch (const GMetronomeError& e) {
#ifndef NDEBUG
      std::cerr << "ListStoreXML: failed to save entry "
                << "('" << e.what() << "')" << std::endl;
#endif
    }
    catch (...) {
#ifndef NDEBUG
      std::cerr << "ListStoreXML: failed to save entry" << std::endl;
#endif
    }
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::list() -> std::vector<Primer>
{
  if (pending_import_ && !import_error_)
    importData();

  std::vector<Primer> primers;
  primers.reserve(t_order_.size());

  std::transform(t_order_.begin(), t_order_.end(), std::back_inserter(primers),
                 [this] (const auto& id) -> Primer {
                   return {id, t_map_[id].header};
                 });

  return primers;
}

template<typename T, typename I, typename H, typename P, typename W>
typename ListStoreXML<T,I,H,P,W>::Type
ListStoreXML<T,I,H,P,W>::load(Identifier id)
{
  if (pending_import_ && !import_error_)
    importData();

  try {
    return t_map_.at(id);
  }
  catch (...) {
    throw GMetronomeError {"ListStoreXML: entry with id '" + id + "' does not exist"};
  }
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::store(Identifier id, const Type& entry)
{
  if (pending_import_ && !import_error_)
    importData();

  if (auto it = t_map_.find(id); it != t_map_.end())
  {
    it->second = entry;
  }
  else
  {
    t_map_[id] = entry;
    t_order_.push_back(id);
  }

  pending_export_ = true;
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::reorder(const std::vector<Identifier>& order)
{
  if (pending_import_ && !import_error_)
    importData();

  assert(order.size() == t_order_.size());

  assert(std::is_permutation(t_order_.begin(), t_order_.end(),
                             order.begin(), order.end()));

  t_order_ = order;
  pending_export_ = true;
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::remove(Identifier id)
{
  if (pending_import_ && !import_error_)
    importData();

  if (auto it = std::find(t_order_.begin(), t_order_.end(), id);
      it != t_order_.end())
  {
    t_order_.erase(it);
  }
  t_map_.erase(id);
  pending_export_ = true;
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::flush()
{
  if (pending_export_)
  {
    if (pending_import_ && !import_error_)
      importData();

    exportData();
  }
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::importData()
{
  Parser parser;
  Glib::Markup::ParseContext context(parser);

  std::array<char, 4096> buffer;

  try {
    bool dedicated_import = !import_path_.empty();
    Glib::RefPtr<Gio::File> file;

    if (dedicated_import)
      file = Gio::File::create_for_path(import_path_);
    else
      file = Gio::File::create_for_path(path_);

    auto input_stream = file->read();
    for (auto bytes_read = input_stream->read(buffer.data(), buffer.size());
         bytes_read > 0;
         bytes_read = input_stream->read(buffer.data(), buffer.size()))
    {
      context.parse(buffer.data(), buffer.data() + bytes_read);
    }
    context.end_parse();
    t_map_ = parser.moveMap();
    t_order_ = parser.moveOrder();
  }
  catch(const Gio::Error& e)
  {
    switch (e.code()) {
    case Gio::Error::NOT_FOUND:
      // ignore (file might not have been created yet)
      break;
    default:
      import_error_ = true;
      throw GMetronomeError { e.what() };
      break;
    }
  }
  catch(const Glib::MarkupError& e)
  {
    import_error_ = true;
    throw GMetronomeError { e.what() };
  }
  catch(...)
  {
    import_error_ = true;
    throw;
  }

  if (!import_path_.empty())
    import_path_.clear();

  pending_import_ = false;
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::exportData()
{
  auto file = Gio::File::create_for_path(path_);

  // Create parent directories, if necessary.
  if ( auto parent = file->get_parent(); parent) {
    try {
      parent->make_directory_with_parents();
    }
    catch (const Gio::Error& e) {
      if (e.code() != Gio::Error::EXISTS)
      {
        export_error_ = true;
        throw GMetronomeError { e.what() };
      }
    }
  }
  // Open output stream, replacing the file if it already exists.
  Glib::RefPtr<Gio::FileOutputStream> ostream;
  static const Gio::FileCreateFlags flags = Gio::FILE_CREATE_PRIVATE;
  try {
    ostream = file->replace(std::string(), false, flags);
  }
  catch (const Gio::Error& e) {
    export_error_ = true;
    throw GMetronomeError { e.what() };
  }

  Writer writer;

  std::string tl_name = std::string(PACKAGE) + "-" + writer.topLevelElementName();
  std::string tl_open_tag = std::string("<") + tl_name + " version=\"" + PACKAGE_VERSION + "\">";
  std::string tl_close_tag = std::string("</") + tl_name + ">";

  assert(ostream);
  ostream->write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  ostream->write(tl_open_tag + "\n");
  for (const auto& id : t_order_)
  {
    writer.writeEntry(ostream, t_map_[id], id);
  }
  ostream->write(tl_close_tag + "\n");
  ostream->flush();
  ostream->close();
}

#endif//GMetronome_ListStoreXML_h
