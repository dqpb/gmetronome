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

#include "AudioBuffer.h"
#include <stdexcept>
#include <map>

namespace audio {
  
  Buffer::Buffer(size_type nbytes, const SampleSpec& spec)
    : data_(nbytes,0),
      spec_(spec)
  {}
  
  Buffer::Buffer(microseconds duration, const SampleSpec& spec)
    : data_(usecsToBytes(duration,spec),0),
      spec_(spec)
  {}

  Buffer::Buffer(byte_container data, const SampleSpec& spec) {
    std::swap(data_,data);
    spec_ = spec;
  }
  
  Buffer::Buffer(const std::string& filename) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  Buffer::Buffer(const Buffer& buffer) {
    data_ = buffer.data_;
    spec_ = buffer.spec_;
  }
  
  Buffer::Buffer(Buffer&& buffer) {
    data_ = std::move(buffer.data_);
    spec_ = std::move(buffer.spec_);
  }

  Buffer::~Buffer() {}
  
  void Buffer::load(const std::string& filename) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  Buffer Buffer::resample(const SampleSpec& spec) {
    throw std::runtime_error(std::string(__FUNCTION__) + " not implemented yet");
  }

  microseconds Buffer::time() const {
    return bytesToUsecs(data_.size(), spec_);
  }

  Buffer& Buffer::operator=(const Buffer& buffer) {
    if (&buffer != this) {
      data_ = buffer.data_;
      spec_ = buffer.spec_;
    }
    return *this;
  }
  
  Buffer& Buffer::operator=(Buffer&& buffer) {
    if (&buffer != this) {
      data_ = std::move(buffer.data_);
      spec_ = std::move(buffer.spec_);
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
    std::swap(data_, buffer.data_);
    std::swap(spec_, buffer.spec_);
  }

  Buffer::size_type Buffer::frames() const {
    auto fs = frameSize(spec_);
    return (fs == 0) ? 0 : data_.size() / fs;
  }

}//namespace audio
