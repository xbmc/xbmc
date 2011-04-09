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
// H.261 Video RTP Sources
// C++ header

#ifndef _H261_VIDEO_RTP_SOURCE_HH
#define _H261_VIDEO_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

class H261VideoRTPSource: public MultiFramedRTPSource {
public:
  static H261VideoRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat = 31,
	    unsigned rtpTimestampFrequency = 90000);

  u_int32_t lastSpecialHeader() const {return fLastSpecialHeader;}

protected:
  virtual ~H261VideoRTPSource();

private:
  H261VideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char rtpPayloadFormat,
			 unsigned rtpTimestampFrequency);
      // called only by createNew()

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  u_int32_t fLastSpecialHeader;
};

#endif
