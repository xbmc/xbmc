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
// Descriptor preceding frames of 'ADU' MP3 streams (for improved loss-tolerance)
// C++ header

#ifndef _MP3_ADU_DESCRIPTOR_HH
#define _MP3_ADU_DESCRIPTOR_HH

// A class for handling the descriptor that begins each ADU frame:
//   (Note: We don't yet implement fragmentation)
class ADUdescriptor {
public:
  // Operations for generating a new descriptor
  static unsigned computeSize(unsigned remainingFrameSize) {
    return remainingFrameSize >= 64 ? 2 : 1;
  }
  static unsigned generateDescriptor(unsigned char*& toPtr, unsigned remainingFrameSize);
   // returns descriptor size; increments "toPtr" afterwards
  static void generateTwoByteDescriptor(unsigned char*& toPtr, unsigned remainingFrameSize);
   // always generates a 2-byte descriptor, even if "remainingFrameSize" is
   // small enough for a 1-byte descriptor

  // Operations for reading a descriptor
  static unsigned getRemainingFrameSize(unsigned char*& fromPtr);
   // increments "fromPtr" afterwards
};

#endif
