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

#ifndef __AUDIO_BUFFER_H__
#define __AUDIO_BUFFER_H__

#include <vector>
#include <string>
#include <chrono>
#include "Audio.h"

namespace audio {

  class Buffer;
  class BufferView;
  class BufferFrameView;
  
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
    
    static constexpr SampleSpec DefaultSpec { SampleFormat::S16LE, 44100, 2 };
    
  public:
    /** Constructs an empty audio buffer with the default sample specification */
    Buffer();

    /** Copy constructor. */
    Buffer(const Buffer& buffer);
    
    /** Move constructor. */
    Buffer(Buffer&& buffer);
    
    /** Constructs a buffer of nbytes zero initialized bytes. */
    Buffer(size_type nbytes, const SampleSpec& spec = DefaultSpec);

    /** Constructs a zero initialized buffer capable of holding 
	the specified time of audio data. */
    Buffer(microseconds duration, const SampleSpec& spec = DefaultSpec);

    /** Constructs a buffer with a byte_container. */
    Buffer(byte_container data, const SampleSpec& spec = DefaultSpec);

    /** Construct the buffer with the contents of an audio file. */
    Buffer(const std::string& filename);

    /** Destructor. */
    ~Buffer();

    /** Returns the byte_container with the audio data. */
    const byte_container& data() const
    { return _data; }
    /** Returns a reference to the sample specification. */
    const SampleSpec& spec() const
    { return _spec; }
    
    void load(const std::string& filename);
    Buffer resample(const SampleSpec& spec);
    microseconds time() const;

    Buffer& operator=(const Buffer&);
    Buffer& operator=(Buffer&&);
    bool operator==(const Buffer&) const;
    bool operator!=(const Buffer&) const;
    
    iterator begin()
    { return _data.begin(); }
    const_iterator begin() const
    { return _data.begin(); }
    const_iterator cbegin() const
    { return _data.cbegin(); }
    iterator end()
    { return _data.end(); }
    const_iterator end() const
    { return _data.end(); }
    const_iterator cend() const
    { return _data.cend(); }    
    
    reference operator[]( size_type pos )
    { return _data[pos]; }
    const_reference operator[]( size_type pos ) const
    { return _data[pos]; }
    
    void swap(Buffer&);
    
    size_type size() const
    { return _data.size(); }
    size_type max_size() const
    { return _data.max_size(); }
    size_type bytes() const
    { return size(); }
    size_type frames() const
    { return _data.size() / frameSize(_spec); }
    bool empty() const
    { return _data.empty(); }
    
    operator BufferView ();
    
  private:
    byte_container _data;
    SampleSpec _spec;
  };

  
  /**
   * A BufferView refers to a Buffer and provides a view to a subregion 
   * of the buffer. It does not copy nor does it gain ownership of any 
   * data of the underlying Buffer. Therefore the Buffer object needs 
   * to exceed the whole livetime of the view.
   */
  class BufferView {
  public:
    using  byte_container  = Buffer::byte_container;
    using  value_type      = byte_container::value_type;
    using  reference       = byte_container::reference;
    using  const_reference = byte_container::const_reference;
    using  difference_type = byte_container::difference_type;
    using  size_type       = byte_container::size_type;
    using  iterator        = byte_container::iterator;
    using  const_iterator  = byte_container::const_iterator;

  public:
    BufferView(const Buffer& buffer);

    BufferView(const Buffer& buffer,
               Buffer::const_iterator begin,
               Buffer::const_iterator end);
    
    BufferView(const Buffer& buffer,
               microseconds start,
               microseconds duration);
    
    BufferView(const BufferView& view,
               Buffer::const_iterator begin,
               Buffer::const_iterator end);

    const Buffer& buffer() const
    { return _buffer; }
    const byte_container& data() const
    { return _buffer.data(); }
    const SampleSpec& spec() const
    { return _buffer.spec(); }

    Buffer resample(const SampleSpec& spec);
    microseconds time() const;
    
    bool operator==(const Buffer&) const;
    bool operator!=(const Buffer&) const;
    
    const_iterator begin() const
    { return _begin; }
    const_iterator cbegin() const
    { return _begin; }
    const_iterator end() const
    { return _end; }
    const_iterator cend() const
    { return _end; }    
    
    const_reference operator[]( size_type pos ) const
    { return *(_begin + pos); }
    
    void swap(BufferView&);
    
    size_type size() const
    { return _end - _begin; }
    size_type max_size() const
    { return _buffer.max_size(); }
    bool empty() const
    { return _begin == _end; }

  protected:
    const Buffer& _buffer;
    const_iterator _begin;
    const_iterator _end;
  };
  
  /* not implemented yet */
  class BufferFrameView {
  public:
    using  byte_container  = Buffer::byte_container;
    using  value_type      = byte_container::value_type;
    using  reference       = byte_container::reference;
    using  const_reference = byte_container::const_reference;
    using  difference_type = byte_container::difference_type;
    using  size_type       = byte_container::size_type;
    
    class iterator {
      // not implemented yet
    };
    class const_iterator {
      // not implemented yet
    };

  public:
    BufferFrameView(const Buffer& buffer);
        
    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cend() const;
    
  private:
    const Buffer& _buffer;
    const_iterator _begin;
    const_iterator _end;
  };

}//namespace audio
#endif//__AUDIO_BUFFER_H__
