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
// MPEG-4 audio, using LATM multiplexing
// C++ header

#ifndef _MPEG4_LATM_AUDIO_RTP_SOURCE_HH
#define _MPEG4_LATM_AUDIO_RTP_SOURCE_HH

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

class MPEG4LATMAudioRTPSource: public MultiFramedRTPSource {
public:
  static MPEG4LATMAudioRTPSource*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency);

  // By default, the LATM data length field is included at the beginning of each
  // returned frame.  To omit this field, call the following:
  void omitLATMDataLengthField();

  Boolean returnedFrameIncludesLATMDataLengthField() const { return fIncludeLATMDataLengthField; }

protected:
  virtual ~MPEG4LATMAudioRTPSource();

private:
  MPEG4LATMAudioRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			  unsigned char rtpPayloadFormat,
			  unsigned rtpTimestampFrequency);
      // called only by createNew()

private:
  // redefined virtual functions:
  virtual Boolean processSpecialHeader(BufferedPacket* packet,
                                       unsigned& resultSpecialHeaderSize);
  virtual char const* MIMEtype() const;

private:
  Boolean fIncludeLATMDataLengthField;
};


// A utility for parsing a "StreamMuxConfig" string
Boolean
parseStreamMuxConfigStr(char const* configStr,
			// result parameters:
			Boolean& audioMuxVersion,
			Boolean& allStreamsSameTimeFraming,
			unsigned char& numSubFrames,
			unsigned char& numProgram,
			unsigned char& numLayer,
			unsigned char*& audioSpecificConfig,
			unsigned& audioSpecificConfigSize);
    // Parses "configStr" as a sequence of hexadecimal digits, representing
    // a "StreamMuxConfig" (as defined in ISO.IEC 14496-3, table 1.21).
    // Returns, in "audioSpecificConfig", a binary representation of
    // the enclosed "AudioSpecificConfig" structure (of size
    // "audioSpecificConfigSize" bytes).  The memory for this is allocated
    // dynamically by this function; the caller is responsible for
    // freeing it.  Other values, that precede "AudioSpecificConfig",
    // are returned in the other parameters.
    // Returns True iff the parsing succeeds.
    // IMPORTANT NOTE: The implementation of this function currently assumes
    // that everything after the first "numLayer" field is an
    // "AudioSpecificConfig".  Therefore, it will not work properly if
    // "audioMuxVersion" != 0, "numProgram" > 0, or "numLayer" > 0.
    // Also, any 'other data' or CRC info will be included at
    // the end of "audioSpecificConfig".

unsigned char* parseStreamMuxConfigStr(char const* configStr,
				       // result parameter:
				       unsigned& audioSpecificConfigSize);
    // A variant of the above that returns just the "AudioSpecificConfig" data
    // (or NULL) if the parsing failed, without bothering with the other
    // result parameters.

unsigned char* parseGeneralConfigStr(char const* configStr,
				     // result parameter:
				     unsigned& configSize);
    // A routine that parses an arbitrary config string, returning
    // the result in binary form.

#endif
