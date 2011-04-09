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
// RTP Sources containing generic QuickTime stream data, as defined in
//     <http://developer.apple.com/quicktime/icefloe/dispatch026.html>
// C++ header

#ifndef _QUICKTIME_GENERIC_RTP_SOURCE_HH
#define _QUICKTIME_GENERIC_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

class QuickTimeGenericRTPSource: public MultiFramedRTPSource {
public:
  static QuickTimeGenericRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat, unsigned rtpTimestampFrequency,
	    char const* mimeTypeString);

  // QuickTime-specific information, set from the QuickTime header
  // in each packet.  This, along with the data following the header,
  // is used by receivers.
  struct QTState {
    char PCK;
    unsigned timescale;
    char* sdAtom;
    unsigned sdAtomSize;
    unsigned short width, height;
    // later add other state as needed #####
  } qtState;

protected:
  virtual ~QuickTimeGenericRTPSource();

private:
  QuickTimeGenericRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			    unsigned char rtpPayloadFormat,
			    unsigned rtpTimestampFrequency,
			    char const* mimeTypeString);
      // called only by createNew()

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  char const* fMIMEtypeString;
};

#endif
