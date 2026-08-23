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
#include <random>

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
  using ItemMap = std::map<Identifier, Type>;
  using OrderVector = std::vector<Identifier>;

  virtual ItemMap moveMap() = 0;
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

  virtual void writeItem(Glib::RefPtr<Gio::FileOutputStream> ostream,
                         const Type& item,
                         const Identifier& id) = 0;
};

/**
 * @class ListStoreXML
 *
 * Implements a cached list store with xml file persistence.
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
  using typename ListStore<T,I,H>::Patch;

  template<typename R>
  using Result = typename ListStore<T,I,H>::template Result<R>;

  using typename ListStore<T,I,H>::Error;

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
  Result<std::vector<Primer>> list() override;
  Result<Type> load(const Identifier& id) override;
  Result<void> store(const Identifier& id, const Type& item) override;
  Result<void> remove(const Identifier& id) override;
  Result<void> update(const Identifier& id, const Patch& patch) override;
  Result<void> reorder(const std::vector<Identifier>& order) override;
  Result<void> flush() override;

private:
  using ItemMap = std::map<Identifier, Type>;
  using OrderVector = std::vector<Identifier>;
  ItemMap t_map_;
  OrderVector t_order_;

  std::string path_;
  std::string import_path_;

  bool pending_import_{true};
  bool import_error_{false};

  bool pending_export_{false};
  bool export_error_{false};

  Result<void> importData() noexcept;
  Result<void> exportData() noexcept;

  Result<Glib::RefPtr<Gio::FileInputStream>>
  openFileInputStream(const std::string& path) noexcept;

  Result<std::pair<ItemMap,OrderVector>>
  parseStream(Glib::RefPtr<Gio::FileInputStream> istream) noexcept;

  Result<Glib::RefPtr<Gio::FileOutputStream>>
  openFileOutputStream(const std::string& path, bool backup = false) noexcept;

  Result<void> writeStream(Glib::RefPtr<Gio::FileOutputStream> ostream) noexcept;

  void printError(const std::string& msg, const Error& e = {}) const;
  void printMessage(const std::string& msg) const;
};

template<typename T, typename I, typename H, typename P, typename W>
ListStoreXML<T,I,H,P,W>::~ListStoreXML()
{ flush(); }

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::list() -> Result<std::vector<Primer>>
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
auto ListStoreXML<T,I,H,P,W>::load(const Identifier& id) -> Result<Type>
{
  if (pending_import_ && !import_error_)
    importData();

  try { return t_map_.at(id); }
  catch (...) {
    return Error {Error::Category::kNotFound, "Item '" + id + "' not found."};
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::store(const Identifier& id, const Type& item) -> Result<void>
{
  if (pending_import_ && !import_error_)
    importData();

  if (auto it = t_map_.find(id); it != t_map_.end()) {
    it->second = item;
  }
  else {
    t_map_[id] = item;
    t_order_.push_back(id);
  }
  pending_export_ = true;
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::remove(const Identifier& id) -> Result<void>
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
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::update(const Identifier& id, const Patch& patch) -> Result<void>
{
  if (pending_import_ && !import_error_)
    importData();

  try {
    patch.apply( t_map_.at(id) );
  }
  catch (const std::out_of_range&) {
    return Error {Error::Category::kNotFound, "Item not found."};
  }
  catch(...) {
    return Error {Error::Category::kUnknown, "Failed to apply patch."};
  }
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::reorder(const std::vector<Identifier>& order) -> Result<void>
{
  if (pending_import_ && !import_error_)
    importData();

  if (!std::is_permutation(t_order_.begin(), t_order_.end(), order.begin(), order.end()))
    return Error {Error::Category::kValidation, "Order is not a permutation."};

  t_order_ = order;
  pending_export_ = true;
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::flush() -> Result<void>
{
  if (pending_export_)
    return exportData();
  else
    return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::openFileInputStream(const std::string& path) noexcept
  -> Result<Glib::RefPtr<Gio::FileInputStream>>
{
  try {
    return Gio::File::create_for_path(path)->read();
  }
  catch (const Gio::Error& e) {
    // ignore 'not found' (file might not have been created yet)
    if (e.code() == Gio::Error::NOT_FOUND)
      return {};
    else
      return Error { Error::Category::kIO, "I/O error.", e.what() };
  }
  catch (...) {
    return Error { Error::Category::kIO, "I/O error."};
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::parseStream(Glib::RefPtr<Gio::FileInputStream> istream) noexcept
  -> Result<std::pair<ItemMap,OrderVector>>
{
  try {
    Parser parser;
    Glib::Markup::ParseContext context(parser);
    std::array<char, 4096> buffer;

    for (auto bytes_read = istream->read(buffer.data(), buffer.size());
         bytes_read > 0;
         bytes_read = istream->read(buffer.data(), buffer.size()))
    {
      context.parse(buffer.data(), buffer.data() + bytes_read);
    }
    context.end_parse();

    return std::make_pair(parser.moveMap(), parser.moveOrder());
  }
  catch (const Gio::Error& e) {
    return Error { Error::Category::kIO, "I/O error.", e.what() };
  }
  catch (const Glib::MarkupError& e) {
    return Error { Error::Category::kParse, "Markup error.", e.what() };
  }
  catch (...) {
    return Error { Error::Category::kUnknown, "Unknown error."};
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::importData() noexcept -> Result<void>
{
  bool dedicated_import = !import_path_.empty();
  const std::string& path = (dedicated_import) ? import_path_ : path_;

  if (auto istream_result = openFileInputStream(path); istream_result)
  {
    if (auto istream = istream_result.value(); istream)
    {
      if (auto parse_result = parseStream(istream); parse_result)
      {
        t_map_ = std::move(parse_result->first);
        t_order_ = std::move(parse_result->second);
      }
      else {
        printError("Failed to parse '" + path + "'.", parse_result.error());
        import_error_ = true;
        return parse_result.error();
      }
    }
    else {} // Ok, file not found.
  }
  else {
    printError("Failed to open '" + path + "'.", istream_result.error());
    import_error_ = true;
    return istream_result.error();
  }

  pending_import_ = false;
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::openFileOutputStream(const std::string& path, bool backup) noexcept
  -> Result<Glib::RefPtr<Gio::FileOutputStream>>
{
  auto file = Gio::File::create_for_path(path_); // never fails

  try {
    try {
      // Create parent directories, if necessary.
      if ( auto parent = file->get_parent(); parent)
        parent->make_directory_with_parents();
    }
    catch (const Gio::Error& e) {
      if (e.code() != Gio::Error::EXISTS)
        throw;
    }
    // Open output stream and replace the file or create a new one.
    // If backup is true, we try to make a backup. If that fails we try
    // again without the backup flag set.
    return file->replace(std::string(), backup, Gio::FILE_CREATE_PRIVATE);
  }
  catch (const Gio::Error& e) {
    if (e.code() == Gio::Error::CANT_CREATE_BACKUP) {
      try {
        return file->replace(std::string(), false, Gio::FILE_CREATE_PRIVATE);
      }
      catch (const Gio::Error& retry_e) {
        return Error { Error::Category::kIO, "I/O error.", retry_e.what() };
      }
    }
    else return Error { Error::Category::kIO, "I/O error.", e.what() };
  }
  catch (...) {
    return Error { Error::Category::kUnknown, "Unknown error." };
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::writeStream(Glib::RefPtr<Gio::FileOutputStream> ostream) noexcept
  -> Result<void>
{
  try {
    Writer writer;

    std::string tl_name = std::string(PACKAGE) + "-" + writer.topLevelElementName();
    std::string tl_open_tag = std::string("<") + tl_name + " version=\"" + PACKAGE_VERSION + "\">";
    std::string tl_close_tag = std::string("</") + tl_name + ">";

    assert(ostream);
    ostream->write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    ostream->write(tl_open_tag + "\n");
    for (const auto& id : t_order_)
    {
      writer.writeItem(ostream, t_map_[id], id);
    }
    ostream->write(tl_close_tag + "\n");
    ostream->flush();
    ostream->close();

    return {};
  }
  catch (const Gio::Error& e) {
    return Error { Error::Category::kSerialization, "Serialization error.", e.what() };
  }
  catch (...) {
    return Error { Error::Category::kUnknown, "Unknown error."};
  }
}

template<typename T, typename I, typename H, typename P, typename W>
auto ListStoreXML<T,I,H,P,W>::exportData() noexcept -> Result<void>
{
  // Open the backing file and try to make a backup if an import error
  // occured previously. Then replace the file with the new content.
  if (auto ostream_result = openFileOutputStream(path_, import_error_); !ostream_result)
  {
    printError("Failed to open '" + path_ + "'.", ostream_result.error());
    export_error_ = true;
    return ostream_result.error();
  }
  else if (auto write_result = writeStream(*ostream_result); !write_result)
  {
    printError("Failed to write '" + path_ + "'.",  write_result.error());
    export_error_ = true;
    return write_result.error();
  }
  pending_export_ = false;
  return {};
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::printError(const std::string& msg, const Error& e) const
{
#ifndef NDEBUG
  std::cerr << "ListStoreXML: " << msg;
  if (!e.what.empty())
    std::cerr << " (" << e.what << ")";
  std::cerr << std::endl;
  if (!e.detail.empty())
    std::cerr << "ListStoreXML: Detail: " << e.detail << std::endl;
#endif
}

template<typename T, typename I, typename H, typename P, typename W>
void ListStoreXML<T,I,H,P,W>::printMessage(const std::string& msg) const
{
#ifndef NDEBUG
  std::cout << "ListStoreXML: " << msg << std::endl;
#endif
}

#endif//GMetronome_ListStoreXML_h
