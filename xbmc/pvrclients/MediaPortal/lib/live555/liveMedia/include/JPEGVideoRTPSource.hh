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
// JPEG Video (RFC 2435) RTP Sources
// C++ header

#ifndef _JPEG_VIDEO_RTP_SOURCE_HH
#define _JPEG_VIDEO_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

#define MAX_JPEG_HEADER_SIZE 1024

class JPEGVideoRTPSource: public MultiFramedRTPSource {
public:
  static JPEGVideoRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat = 26,
	    unsigned rtpPayloadFrequency = 90000,
	    unsigned defaultWidth = 0, unsigned defaultHeight = 0);

protected:
  virtual ~JPEGVideoRTPSource();

private:
  JPEGVideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		     unsigned char rtpPayloadFormat,
		     unsigned rtpTimestampFrequency,
		     unsigned defaultWidth, unsigned defaultHeight);
      // called only by createNew()

  // Image dimensions from the SDP description, if any
  unsigned fDefaultWidth, fDefaultHeight;

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);

  virtual char const* MIMEtype() const;
};

#endif
