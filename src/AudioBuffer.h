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

#ifndef GMetronome_AudioBuffer_h
#define GMetronome_AudioBuffer_h

#include <vector>
#include <string>
#include <chrono>
#include "Audio.h"

namespace audio {

  /**
   * A Buffer aggregates sample data and a sample specification.
   */
  class Buffer {
  public:
    using  byte_container  = std::vector<uint8_t>;
    using  allocator_type  = byte_container::allocator_type;
    using  value_type      = byte_container::value_type;
    using  reference       = byte_container::reference;
    using  const_reference = byte_container::const_reference;
    using  difference_type = byte_container::difference_type;
    using  size_type       = byte_container::size_type;
    using  iterator        = byte_container::iterator;
    using  const_iterator  = byte_container::const_iterator;

  public:

    /** Constructs a buffer of nbytes zero initialized bytes. */
    Buffer(size_type nbytes = 0, const StreamSpec& spec = kDefaultSpec);

    /** Constructs a zero initialized buffer capable of holding
        the specified time of audio data. */
    explicit Buffer(microseconds duration, const StreamSpec& spec = kDefaultSpec);

    /** Constructs a buffer with a byte_container. */
    explicit Buffer(byte_container data, const StreamSpec& spec = kDefaultSpec);

    /** Returns the byte_container with the audio data. */
    const byte_container& data() const
    { return data_; }

    /** Returns a reference to the sample specification. */
    const StreamSpec& spec() const
    { return spec_; }

    Buffer resample(const StreamSpec& spec);
    microseconds time() const;

    bool operator==(const Buffer&) const;
    bool operator!=(const Buffer&) const;

    iterator begin()
    { return data_.begin(); }
    const_iterator begin() const
    { return data_.begin(); }
    const_iterator cbegin() const
    { return data_.cbegin(); }
    iterator end()
    { return data_.end(); }
    const_iterator end() const
    { return data_.end(); }
    const_iterator cend() const
    { return data_.cend(); }

    reference operator[]( size_type pos )
    { return data_[pos]; }
    const_reference operator[]( size_type pos ) const
    { return data_[pos]; }

    void swap(Buffer&);

    size_type size() const
    { return data_.size(); }
    size_type max_size() const
    { return data_.max_size(); }
    size_type bytes() const
    { return size(); }
    size_type frames() const;
    bool empty() const
    { return data_.empty(); }

  private:
    byte_container data_;
    StreamSpec spec_;
  };

}//namespace audio
#endif//GMetronome_AudioBuffer_h
