// //////////////////////////////////////////////////////////
// sha3.h
// Copyright (c) 2014 Stephan Brumme. All rights reserved.
// see http://create.stephan-brumme.com/disclaimer.html
//

#pragma once

//#include "hash.h"
#include <string>

// define fixed size integer types
#ifdef _MSC_VER
// Windows
typedef unsigned __int8  uint8_t;
typedef unsigned __int64 uint64_t;
#else
// GCC
#include <stdint.h>
#endif


/// compute SHA3 hash
/** Usage:
    SHA3 sha3;
    std::string myHash  = sha3("Hello World");     // std::string
    std::string myHash2 = sha3("How are you", 11); // arbitrary data, 11 bytes

    // or in a streaming fashion:

    SHA3 sha3;
    while (more data available)
      sha3.add(pointer to fresh data, number of new bytes);
    std::string myHash3 = sha3.getHash();
  */
class SHA3 //: public Hash
{
public:
  /// algorithm variants
  enum Bits { Bits224 = 224, Bits256 = 256, Bits384 = 384, Bits512 = 512 };

  /// same as reset()
  explicit SHA3(Bits bits = Bits256);

  /// compute hash of a memory block
  std::string operator()(const void* data, size_t numBytes);
  /// compute hash of a string, excluding final zero
  std::string operator()(const std::string& text);

  /// add arbitrary number of bytes
  void add(const void* data, size_t numBytes);

  /// return latest hash as hex characters
  std::string getHash();

  /// restart
  void reset();

private:
  /// process a full block
  void processBlock(const void* data);
  /// process everything left in the internal buffer
  void processBuffer();

  /// 1600 bits, stored as 25x64 bit, BlockSize is no more than 1152 bits (Keccak224)
  enum { StateSize    = 1600 / (8 * 8),
         MaxBlockSize =  200 - 2 * (224 / 8) };

  /// hash
  uint64_t m_hash[StateSize];
  /// size of processed data in bytes
  uint64_t m_numBytes;
  /// block size (less or equal to MaxBlockSize)
  size_t   m_blockSize;
  /// valid bytes in m_buffer
  size_t   m_bufferSize;
  /// bytes not processed yet
  uint8_t  m_buffer[MaxBlockSize];
  /// variant
  Bits     m_bits;
};
