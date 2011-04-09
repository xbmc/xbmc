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
// MPEG4-GENERIC ("audio", "video", or "application") RTP stream sources
// C++ header

#ifndef _MPEG4_GENERIC_RTP_SOURCE_HH
#define _MPEG4_GENERIC_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

#define SPECIAL_HEADER_BUFFER_SIZE 1000

class MPEG4GenericRTPSource: public MultiFramedRTPSource {
public:
  static MPEG4GenericRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency,
	    char const* mediumName,
	    char const* mode, unsigned sizeLength, unsigned indexLength,
	    unsigned indexDeltaLength
	    // add other parameters later
	    );
  // mediumName is "audio", "video", or "application"
  // it *cannot* be NULL

protected:
  virtual ~MPEG4GenericRTPSource();

private:
  MPEG4GenericRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			unsigned char rtpPayloadFormat,
			unsigned rtpTimestampFrequency,
			char const* mediumName,
			char const* mode,
			unsigned sizeLength, unsigned indexLength,
			unsigned indexDeltaLength
			);
      // called only by createNew()

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  char* fMIMEType;

  char* fMode;
  unsigned fSizeLength, fIndexLength, fIndexDeltaLength;
  unsigned fNumAUHeaders; // in the most recently read packet
  unsigned fNextAUHeader; // index of the next AU Header to read
  struct AUHeader* fAUHeaders;

  friend class MPEG4GenericBufferedPacket;
};



// A function that looks up the sampling frequency from an
// "AudioSpecificConfig" string.  (0 means 'unknown')
unsigned samplingFrequencyFromAudioSpecificConfig(char const* configStr);

#endif
