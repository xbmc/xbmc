/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Bit Vector data structure
// Implementation

#include "BitVector.hh"

BitVector::BitVector(unsigned char* baseBytePtr,
		     unsigned baseBitOffset,
		     unsigned totNumBits) {
  setup(baseBytePtr, baseBitOffset, totNumBits);
}

void BitVector::setup(unsigned char* baseBytePtr,
		      unsigned baseBitOffset,
		      unsigned totNumBits) {
  fBaseBytePtr = baseBytePtr;
  fBaseBitOffset = baseBitOffset;
  fTotNumBits = totNumBits;
  fCurBitIndex = 0;
}

static unsigned char const singleBitMask[8]
    = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

#define MAX_LENGTH 32

void BitVector::putBits(unsigned from, unsigned numBits) {
  unsigned char tmpBuf[4];
  unsigned overflowingBits = 0;

  if (numBits > MAX_LENGTH) {
    numBits = MAX_LENGTH;
  }

  if (numBits > fTotNumBits - fCurBitIndex) {
    overflowingBits = numBits - (fTotNumBits - fCurBitIndex);
  }

  tmpBuf[0] = (unsigned char)(from>>24);
  tmpBuf[1] = (unsigned char)(from>>16);
  tmpBuf[2] = (unsigned char)(from>>8);
  tmpBuf[3] = (unsigned char)from;

  shiftBits(fBaseBytePtr, fBaseBitOffset + fCurBitIndex, /* to */
	    tmpBuf, MAX_LENGTH - numBits, /* from */
	    numBits - overflowingBits /* num bits */);
  fCurBitIndex += numBits - overflowingBits;
}


void BitVector::put1Bit(unsigned bit) {
  // The following is equivalent to "putBits(..., 1)", except faster:
  if (fCurBitIndex >= fTotNumBits) { /* overflow */
    return;
  } else {
    unsigned totBitOffset = fBaseBitOffset + fCurBitIndex++;
    unsigned char mask = singleBitMask[totBitOffset%8];
    if (bit) {
      fBaseBytePtr[totBitOffset/8] |= mask;
    } else {
      fBaseBytePtr[totBitOffset/8] &=~ mask;
    }
  }
}


unsigned BitVector::getBits(unsigned numBits) {
  unsigned char tmpBuf[4];
  unsigned overflowingBits = 0;

  if (numBits > MAX_LENGTH) {
    numBits = MAX_LENGTH;
  }

  if (numBits > fTotNumBits - fCurBitIndex) {
    overflowingBits = numBits - (fTotNumBits - fCurBitIndex);
  }

  shiftBits(tmpBuf, 0, /* to */
	    fBaseBytePtr, fBaseBitOffset + fCurBitIndex, /* from */
	    numBits - overflowingBits /* num bits */);
  fCurBitIndex += numBits - overflowingBits;

  unsigned result
    = (tmpBuf[0]<<24) | (tmpBuf[1]<<16) | (tmpBuf[2]<<8) | tmpBuf[3];
  result >>= (MAX_LENGTH - numBits); // move into low-order part of word
  result &= (0xFFFFFFFF << overflowingBits); // so any overflow bits are 0
  return result;
}

unsigned BitVector::get1Bit() {
  // The following is equivalent to "getBits(1)", except faster:

  if (fCurBitIndex >= fTotNumBits) { /* overflow */
    return 0;
  } else {
    unsigned totBitOffset = fBaseBitOffset + fCurBitIndex++;
    unsigned char curFromByte = fBaseBytePtr[totBitOffset/8];
    unsigned result = (curFromByte >> (7-(totBitOffset%8))) & 0x01;
    return result;
  }
}

void BitVector::skipBits(unsigned numBits) {
  if (numBits > fTotNumBits - fCurBitIndex) { /* overflow */
    fCurBitIndex = fTotNumBits;
  } else {
    fCurBitIndex += numBits;
  }
}


void shiftBits(unsigned char* toBasePtr, unsigned toBitOffset,
	       unsigned char const* fromBasePtr, unsigned fromBitOffset,
	       unsigned numBits) {
  /* Note that from and to may overlap, if from>to */
  unsigned char const* fromBytePtr = fromBasePtr + fromBitOffset/8;
  unsigned fromBitRem = fromBitOffset%8;
  unsigned char* toBytePtr = toBasePtr + toBitOffset/8;
  unsigned toBitRem = toBitOffset%8;

  while (numBits-- > 0) {
    unsigned char fromBitMask = singleBitMask[fromBitRem];
    unsigned char fromBit = (*fromBytePtr)&fromBitMask;
    unsigned char toBitMask = singleBitMask[toBitRem];

    if (fromBit != 0) {
      *toBytePtr |= toBitMask;
    } else {
      *toBytePtr &=~ toBitMask;
    }

    if (++fromBitRem == 8) {
      ++fromBytePtr;
      fromBitRem = 0;
    }
    if (++toBitRem == 8) {
      ++toBytePtr;
      toBitRem = 0;
    }
  }
}
