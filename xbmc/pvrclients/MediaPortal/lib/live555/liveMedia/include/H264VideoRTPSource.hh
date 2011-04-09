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
// H.264 Video RTP Sources
// C++ header

#ifndef _H264_VIDEO_RTP_SOURCE_HH
#define _H264_VIDEO_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

#define SPECIAL_HEADER_BUFFER_SIZE 1000

class H264VideoRTPSource: public MultiFramedRTPSource {
public:
  static H264VideoRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency = 90000);

private:
  H264VideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char rtpPayloadFormat,
			 unsigned rtpTimestampFrequency);
      // called only by createNew()

  virtual ~H264VideoRTPSource();

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  friend class H264BufferedPacket;
  unsigned char fCurPacketNALUnitType;
};

class SPropRecord {
public:
  ~SPropRecord() { delete[] sPropBytes; }

  unsigned sPropLength; // in bytes
  unsigned char* sPropBytes;
};

SPropRecord* parseSPropParameterSets(char const* sPropParameterSetsStr,
				     // result parameter:
				     unsigned& numSPropRecords);
    // Returns the binary value of each 'parameter set' specified in a
    // "sprop-parameter-sets" string (in the SDP description for a H.264/RTP stream).
    // The value is returned as an array (length "numSPropRecords") of "SPropRecord"s.
    // This array is dynamically allocated by this routine, and must be delete[]d by the caller.

#endif
