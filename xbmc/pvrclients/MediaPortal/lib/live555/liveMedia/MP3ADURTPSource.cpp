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
// RTP source for 'ADUized' MP3 frames ("mpa-robust")
// Implementation

#include "MP3ADURTPSource.hh"
#include "MP3ADUdescriptor.hh"

////////// ADUBufferedPacket and ADUBufferedPacketFactory //////////

class ADUBufferedPacket: public BufferedPacket {
private: // redefined virtual functions
  virtual unsigned nextEnclosedFrameSize(unsigned char*& framePtr,
                                 unsigned dataSize);
};

class ADUBufferedPacketFactory: public BufferedPacketFactory {
private: // redefined virtual functions
  virtual BufferedPacket* createNewPacket(MultiFramedRTPSource* ourSource);
};

///////// MP3ADURTPSource implementation ////////

MP3ADURTPSource*
MP3ADURTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			   unsigned char rtpPayloadFormat,
			   unsigned rtpTimestampFrequency) {
   return new MP3ADURTPSource(env, RTPgs, rtpPayloadFormat,
			      rtpTimestampFrequency);
}

MP3ADURTPSource::MP3ADURTPSource(UsageEnvironment& env, Groupsock* RTPgs,
				 unsigned char rtpPayloadFormat,
				 unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency,
			 new ADUBufferedPacketFactory) {
}

MP3ADURTPSource::~MP3ADURTPSource() {
}

char const* MP3ADURTPSource::MIMEtype() const {
  return "audio/MPA-ROBUST";
}

////////// ADUBufferedPacket and ADUBufferredPacketFactory implementation

unsigned ADUBufferedPacket
::nextEnclosedFrameSize(unsigned char*& framePtr, unsigned dataSize) {
  // Return the size of the next MP3 'ADU', on the assumption that
  // the input data is ADU-encoded MP3 frames.
  unsigned char* frameDataPtr = framePtr;
  unsigned remainingFrameSize
    = ADUdescriptor::getRemainingFrameSize(frameDataPtr);
  unsigned descriptorSize = (unsigned)(frameDataPtr - framePtr);
  unsigned fullADUSize = descriptorSize + remainingFrameSize;

  return (fullADUSize <= dataSize) ? fullADUSize : dataSize;
}

BufferedPacket* ADUBufferedPacketFactory
::createNewPacket(MultiFramedRTPSource* /*ourSource*/) {
  return new ADUBufferedPacket;
}
