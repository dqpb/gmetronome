/*
 * Copyright (C) 2017 The GMetronome Team
 * 
 * This file is part of BookReader.
 *
 * BookReader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BookReader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BookReader.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AudioBuffer.h"
#include <stdexcept>
#include <map>

namespace audio {
  
  constexpr SampleSpec Buffer::DefaultSpec;
  
  Buffer::Buffer()
    : _data(),
      _spec(DefaultSpec)
  {}
  
  Buffer::Buffer(const Buffer& buffer) {
    _data = buffer._data;
    _spec = buffer._spec;
  }
  
  Buffer::Buffer(Buffer&& buffer) {
    _data = std::move(buffer._data);
    _spec = std::move(buffer._spec);
  }
  
  Buffer::Buffer(size_type nbytes, const SampleSpec& spec)
    : _data(nbytes,0),
      _spec(spec)
  {}
  
  Buffer::Buffer(microseconds duration, const SampleSpec& spec)
    : _data(usecsToBytes(duration,spec),0),
      _spec(spec)
  {}

  Buffer::Buffer(byte_container data, const SampleSpec& spec) {
    std::swap(_data,data);
    _spec = spec;
  }
  
  Buffer::Buffer(const std::string& filename) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  Buffer::~Buffer() {}
  
  void Buffer::load(const std::string& filename) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  Buffer Buffer::resample(const SampleSpec& spec) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  microseconds Buffer::time() const {
    return bytesToUsecs(_data.size(), _spec);
  }

  Buffer& Buffer::operator=(const Buffer& buffer) {
    if (&buffer != this) {
      _data = buffer._data;
      _spec = buffer._spec;
    }
    return *this;
  }
  
  Buffer& Buffer::operator=(Buffer&& buffer) {
    if (&buffer != this) {
      _data = std::move(buffer._data);
      _spec = std::move(buffer._spec);
    }
    return *this;
  }

  bool Buffer::operator==(const Buffer&) const {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }
  bool Buffer::operator!=(const Buffer&) const {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }
  
  void Buffer::swap(Buffer& buffer) {
    std::swap(_data, buffer._data);
    std::swap(_spec, buffer._spec);
  }

  Buffer::operator BufferView () {
    return BufferView(*this);
  }


  BufferView::BufferView(const Buffer& buffer)
    : BufferView(buffer, buffer.begin(), buffer.end())
  {}

  BufferView::BufferView(const Buffer& buffer,
                         Buffer::const_iterator begin,
                         Buffer::const_iterator end)
    : _buffer(buffer),
      _begin(begin),
      _end(end)
  {}
  
  BufferView::BufferView(const Buffer& buffer,
                         microseconds start,
                         microseconds duration)
    : _buffer(buffer)
  {
    if (start > buffer.time())
      _begin = buffer.end();
    else
      _begin = buffer.begin() + usecsToBytes(start, buffer.spec());
    
    if ((start + duration) > buffer.time())
      _end = buffer.end();
    else
      _end = _begin + usecsToBytes(duration, buffer.spec());
  }
  
  BufferView::BufferView(const BufferView& view,
                         Buffer::const_iterator begin,
                         Buffer::const_iterator end)
    : _buffer(view._buffer),
      _begin(begin),
      _end(end)
  {}
  
  Buffer BufferView::resample(const SampleSpec& spec) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  microseconds BufferView::time() const {
    return bytesToUsecs(size(), spec());
  }

  bool BufferView::operator==(const Buffer&) const {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }
  
  bool BufferView::operator!=(const Buffer&) const {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }
  
  void BufferView::swap(BufferView&) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }
  
}//namespace audio
