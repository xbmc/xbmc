#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <string.h>
#include <string>
#include <memory>

namespace XbmcCommons
{
  class BufferException
  {
    std::string message;

  public:
    BufferException(const char* message_) : message(message_) {}
  };

  /**
   * This class is based on the java java.nio.Buffer class however, it 
   *  does not implement the 'mark' functionality. 
   *
   * [ the following is borrowed from the javadocs for java.nio.Buffer 
   * where it applies to this class]:
   *
   * A buffer is a linear, finite sequence of elements of a unspecified types.
   * Aside from its content, the essential properties of a buffer are its capacity, 
   *   limit, and position:
   *    A buffer's capacity is the number of elements it contains. The capacity 
   *      of a buffer is never negative and never changes.
   *
   *    A buffer's limit is the index of the first element that should not be 
   *      read or written. A buffer's limit is never negative and is never greater 
   *      than its capacity.
   *
   *     A buffer's position is the index of the next element to be read or written. 
   *       A buffer's position is never negative and is never greater than its limit.
   *
   * Invariants:
   *
   * The following invariant holds for the mark, position, limit, and capacity values:
   *
   *     0 <= mark <= position <= limit <= capacity 
   *
   * A newly-created buffer always has a position of zero and a limit set to the 
   * capacity. The initial content of a buffer is, in general, undefined. 
   *
   * Example:
   *  Buffer buffer(1024);
   *  buffer.putInt(1).putString("hello there").putLongLong( ((long long)2)^40 );
   *  buffer.flip();
   *  std::cout << "buffer contents:" << buffer.getInt() << ", ";
   *  std::cout << buffer.getCharPointerDirect() << ", ";
   *  std::cout << buffer.getLongLong() << std::endl;
   *
   * Note: the 'gets' are sensitive to the order-of-operations. Therefore, while
   *  the above is correct, it would be wrong to chain the output as follows:
   *
   *  std::cout << "buffer contents:" << buffer.getInt() << ", " << std::cout
   *      << buffer.getCharPointerDirect() << ", " << buffer.getLongLong() 
   *      << std::endl;
   *
   * This would result in the get's executing from right to left and therefore would
   *  produce totally erroneous results. This is also a problem when the values are
   *  passed to a method as in:
   *
   * printf("buffer contents: %d, \"%s\", %ll\n", buffer.getInt(), 
   *         buffer.getCharPointerDirect(), buffer.getLongLong());
   *
   * This would also produce erroneous results as they get's will be evaluated
   *   from right to left in the parameter list of printf.
   */
  class Buffer
  {
    std::shared_ptr<unsigned char> bufferRef;
    unsigned char* buffer;
    size_t mposition;
    size_t mcapacity;
    size_t mlimit;

    inline void check(int count) const
    { 
      if ((mposition + count) > mlimit) 
        throw BufferException("Buffer buffer overflow: Cannot add more data to the Buffer's buffer.");
    }

  public:
    /**
     * Construct an uninitialized buffer instance, perhaps as an lvalue.
     */
    inline Buffer() : buffer(NULL), mposition(0), mcapacity(0), mlimit(0) { clear(); }

    /**
     * Construct a buffer given an externally managed memory buffer.
     * The ownership of the buffer is assumed to be the code that called
     * this constructor, therefore the Buffer descrutor will not free it.
     *
     * The newly constructed buffer is considered empty and is ready to
     * have data written into it.
     *
     * If you want to read from the buffer you just created, you can use:
     *
     * Buffer b = Buffer(buf,bufSize).forward(bufSize).flip();
     */
    inline Buffer(void* buffer_, size_t bufferSize) : buffer((unsigned char*)buffer_), mcapacity(bufferSize) 
    {
      clear();
    }

    /**
     * Construct a buffer buffer using the size buffer provided. The
     * buffer will be internally managed and potentiall shared with 
     * other Buffer instances. It will be freed upon destruction of
     * the last Buffer that references it.
     */
    inline Buffer(size_t bufferSize) : buffer(bufferSize ? new unsigned char[bufferSize] : NULL), mcapacity(bufferSize)
    { 
      clear(); 
      bufferRef.reset(buffer, std::default_delete<unsigned char[]>());
    }

    /**
     * Copy another buffer. This is a "shallow copy" and therefore
     * shares the underlying data buffer with the Buffer it is a copy
     * of. Changes made to the data through this buffer will be seen
     * in the source buffer and vice/vrs. However, each buffer maintains
     * it's own indexing.
     */
    inline Buffer(const Buffer& buf) : bufferRef(buf.bufferRef), buffer(buf.buffer), 
      mposition(buf.mposition), mcapacity(buf.mcapacity), mlimit(buf.mlimit) { }

    inline ~Buffer() { }

    /**
     * Copy another buffer. This is a "shallow copy" and therefore
     * shares the underlying data buffer with the Buffer it is a copy
     * of. Changes made to the data through this buffer will be seen
     * in the source buffer and vice/vrs. However, each buffer maintains
     * it's own indexing.
     */
    inline Buffer& operator=(const Buffer& buf)
    {
      buffer = buf.buffer;
      bufferRef = buf.bufferRef;
      mcapacity = buf.mcapacity;
      mlimit = buf.mlimit;
      return *this;
    }

    inline Buffer& allocate(size_t bufferSize)
    {
      buffer = bufferSize ? new unsigned char[bufferSize] : NULL;
      bufferRef.reset(buffer, std::default_delete<unsigned char[]>());
      mcapacity = bufferSize;
      clear();
      return *this;
    }

    /**
     * Flips this buffer. The limit is set to the current position 
     *   and then the position is set to zero. 
     *
     * After a sequence of channel-read or put operations, invoke this 
     *   method to prepare for a sequence of channel-write or relative 
     *   get operations. For example:
     *
     * buf.put(magic);    // Prepend header
     * in.read(buf);      // Read data into rest of buffer
     * buf.flip();        // Flip buffer
     * out.write(buf);    // Write header + data to channel
     *
     * This is used to prepare the Buffer for reading from after
     *  it has been written to.
     */
    inline Buffer& flip() { mlimit = mposition; mposition = 0; return *this; }

    /**
     *Clears this buffer. The position is set to zero, the limit 
     *  is set to the capacity.
     *
     * Invoke this method before using a sequence of channel-read 
     *  or put operations to fill this buffer. For example:
     *
     *     buf.clear();     // Prepare buffer for reading
     *     in.read(buf);    // Read data
     *
     * This method does not actually erase the data in the buffer, 
     *  but it is named as if it did because it will most often be used 
     *  in situations in which that might as well be the case. 
     */
    inline Buffer& clear() { mlimit = mcapacity; mposition = 0; return *this; }

    /**
     * This method resets the position to the beginning of the buffer
     *  so that it can be either reread or written to all over again.
     */
    inline Buffer& rewind() { mposition = 0; return *this; }

    /**
     * This method provides for the remaining number of bytes
     *  that can be read out of the buffer or written into the
     *  buffer before it's finished.
     */
    inline size_t remaining() { return mlimit - mposition; }

    inline Buffer& put(const void* src, size_t bytes)
    { check(bytes); memcpy( buffer + mposition, src, bytes); mposition += bytes; return *this; }
    inline Buffer& get(void* dest, size_t bytes)
    { check(bytes); memcpy( dest, buffer + mposition, bytes); mposition += bytes; return *this; }

    inline unsigned char* array() { return buffer; }
    inline unsigned char* curPosition() { return buffer + mposition; }
    inline Buffer& setPosition(size_t position) { mposition = position; return *this; }
    inline Buffer& forward(size_t positionIncrement)
    { check(positionIncrement); mposition += positionIncrement; return *this; }

    inline size_t limit() const { return mlimit; }
    inline size_t capacity() const { return mcapacity; }
    inline size_t position() const { return mposition; }

#define DEFAULTBUFFERRELATIVERW(name,type) \
    inline Buffer& put##name(const type & val) { return put(&val, sizeof(type)); } \
    inline type get##name() { type ret; get(&ret, sizeof(type)); return ret; }

    DEFAULTBUFFERRELATIVERW(Bool,bool);
    DEFAULTBUFFERRELATIVERW(Int,int);
    DEFAULTBUFFERRELATIVERW(Char,char);
    DEFAULTBUFFERRELATIVERW(Long,long);
    DEFAULTBUFFERRELATIVERW(Float,float);
    DEFAULTBUFFERRELATIVERW(Double,double);
    DEFAULTBUFFERRELATIVERW(Pointer,void*);
    DEFAULTBUFFERRELATIVERW(LongLong,long long);
#undef DEFAULTBUFFERRELATIVERW

    inline Buffer& putString(const char* str) { size_t len = strlen(str) + 1; check(len); put(str, len); return (*this); }
    inline Buffer& putString(const std::string& str) { size_t len = str.length() + 1; check(len); put(str.c_str(), len); return (*this); }

    inline std::string getString() { std::string ret((const char*)(buffer + mposition)); size_t len = ret.length() + 1; check(len); mposition += len; return ret; }
    inline std::string getString(size_t length)
    { 
      check(length); 
      std::string ret((const char*)(buffer + mposition),length);
      mposition += length;
      return ret;
    }
    inline char* getCharPointerDirect() { char* ret = (char*)(buffer + mposition); size_t len = strlen(ret) + 1; check(len); mposition += len; return ret; }

  };

}

