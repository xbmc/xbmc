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
// A RTP source for a simple RTP payload format that
//     - doesn't have any special headers following the RTP header
//       (if necessary, the "offset" parameter can be used to specify a
//        special header that we just skip over)
//     - doesn't have any special framing apart from the packet data itself
// C++ header

#ifndef _SIMPLE_RTP_SOURCE_HH
#define _SIMPLE_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

class SimpleRTPSource: public MultiFramedRTPSource {
public:
  static SimpleRTPSource* createNew(UsageEnvironment& env, Groupsock* RTPgs,
				    unsigned char rtpPayloadFormat,
				    unsigned rtpTimestampFrequency,
				    char const* mimeTypeString,
				    unsigned offset = 0,
				    Boolean doNormalMBitRule = True);
  // "doNormalMBitRule" means: If the medium is video, use the RTP "M"
  // bit on each incoming packet to indicate the last (or only) fragment
  // of a frame.  (Otherwise, ignore the "M" bit.)

protected:
  virtual ~SimpleRTPSource();

protected:
  SimpleRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  unsigned rtpTimestampFrequency,
		  char const* mimeTypeString, unsigned offset,
		  Boolean doNormalMBitRule);
      // called only by createNew()

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  char const* fMIMEtypeString;
  unsigned fOffset;
  Boolean fUseMBitForFrameEnd;
};

#endif
