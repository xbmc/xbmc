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
// Implementation

#include "MPEG4LATMAudioRTPSource.hh"

////////// LATMBufferedPacket and LATMBufferedPacketFactory //////////

class LATMBufferedPacket: public BufferedPacket {
public:
  LATMBufferedPacket(Boolean includeLATMDataLengthField);
  virtual ~LATMBufferedPacket();

private: // redefined virtual functions
  virtual unsigned nextEnclosedFrameSize(unsigned char*& framePtr,
                                 unsigned dataSize);

private:
  Boolean fIncludeLATMDataLengthField;
};

class LATMBufferedPacketFactory: public BufferedPacketFactory {
private: // redefined virtual functions
  virtual BufferedPacket* createNewPacket(MultiFramedRTPSource* ourSource);
};

///////// MPEG4LATMAudioRTPSource implementation ////////

MPEG4LATMAudioRTPSource*
MPEG4LATMAudioRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
				   unsigned char rtpPayloadFormat,
				   unsigned rtpTimestampFrequency) {
  return new MPEG4LATMAudioRTPSource(env, RTPgs, rtpPayloadFormat,
				     rtpTimestampFrequency);
}

MPEG4LATMAudioRTPSource
::MPEG4LATMAudioRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			  unsigned char rtpPayloadFormat,
			  unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency,
			 new LATMBufferedPacketFactory),
    fIncludeLATMDataLengthField(True) {
}

MPEG4LATMAudioRTPSource::~MPEG4LATMAudioRTPSource() {
}

void MPEG4LATMAudioRTPSource::omitLATMDataLengthField() {
  fIncludeLATMDataLengthField = False;
}

Boolean MPEG4LATMAudioRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  fCurrentPacketBeginsFrame = fCurrentPacketCompletesFrame;
          // whether the *previous* packet ended a frame

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

  // There is no special header
  resultSpecialHeaderSize = 0;
  return True;
}

char const* MPEG4LATMAudioRTPSource::MIMEtype() const {
  return "audio/MP4A-LATM";
}


////////// LATMBufferedPacket and LATMBufferedPacketFactory implementation

LATMBufferedPacket::LATMBufferedPacket(Boolean includeLATMDataLengthField)
  : fIncludeLATMDataLengthField(includeLATMDataLengthField) {
}

LATMBufferedPacket::~LATMBufferedPacket() {
}

unsigned LATMBufferedPacket
::nextEnclosedFrameSize(unsigned char*& framePtr, unsigned dataSize) {
  // Look at the LATM data length byte(s), to determine the size
  // of the LATM payload.
  unsigned resultFrameSize = 0;
  unsigned i;
  for (i = 0; i < dataSize; ++i) {
    resultFrameSize += framePtr[i];
    if (framePtr[i] != 0xFF) break;
  }
  ++i;
  if (fIncludeLATMDataLengthField) {
    resultFrameSize += i;
  } else {
    framePtr += i;
    dataSize -= i;
  }

  return (resultFrameSize <= dataSize) ? resultFrameSize : dataSize;
}

BufferedPacket* LATMBufferedPacketFactory
::createNewPacket(MultiFramedRTPSource* ourSource) {
  MPEG4LATMAudioRTPSource* source = (MPEG4LATMAudioRTPSource*)ourSource;
  return new LATMBufferedPacket(source->returnedFrameIncludesLATMDataLengthField());
}


////////// parseStreamMuxConfigStr() implementation //////////

static Boolean getNibble(char const*& configStr,
			 unsigned char& resultNibble) {
  char c = configStr[0];
  if (c == '\0') return False; // we've reached the end

  if (c >= '0' && c <= '9') {
    resultNibble = c - '0';
  } else if (c >= 'A' && c <= 'F') {
    resultNibble = 10 + c - 'A';
  } else if (c >= 'a' && c <= 'f') {
    resultNibble = 10 + c - 'a';
  } else {
    return False;
  }

  ++configStr; // move to the next nibble
  return True;
}

static Boolean getByte(char const*& configStr, unsigned char& resultByte) {
  resultByte = 0; // by default, in case parsing fails

  unsigned char firstNibble;
  if (!getNibble(configStr, firstNibble)) return False;
  resultByte = firstNibble<<4;

  unsigned char secondNibble = 0;
  if (!getNibble(configStr, secondNibble) && configStr[0] != '\0') {
    // There's a second nibble, but it's malformed
    return False;
  }
  resultByte |= secondNibble;

  return True;
}

Boolean
parseStreamMuxConfigStr(char const* configStr,
                        // result parameters:
                        Boolean& audioMuxVersion,
                        Boolean& allStreamsSameTimeFraming,
                        unsigned char& numSubFrames,
                        unsigned char& numProgram,
                        unsigned char& numLayer,
                        unsigned char*& audioSpecificConfig,
                        unsigned& audioSpecificConfigSize) {
  // Set default versions of the result parameters:
  audioMuxVersion = 0;
  allStreamsSameTimeFraming = 1;
  numSubFrames = numProgram = numLayer = 0;
  audioSpecificConfig = NULL;
  audioSpecificConfigSize = 0;

  do {
    if (configStr == NULL) break;

    unsigned char nextByte;

    if (!getByte(configStr, nextByte)) break;
    audioMuxVersion = (nextByte&0x80)>>7;
    if (audioMuxVersion != 0) break;

    allStreamsSameTimeFraming = (nextByte&0x40)>>6;
    numSubFrames = (nextByte&0x3F);

    if (!getByte(configStr, nextByte)) break;
    numProgram = (nextByte&0xF0)>>4;

    numLayer = (nextByte&0x0E)>>1;

    // The one remaining bit, and the rest of the string,
    // are used for "audioSpecificConfig":
    unsigned char remainingBit = nextByte&1;

    unsigned ascSize = (strlen(configStr)+1)/2 + 1;
    audioSpecificConfig = new unsigned char[ascSize];

    Boolean parseSuccess;
    unsigned i = 0;
    do {
      nextByte = 0;
      parseSuccess = getByte(configStr, nextByte);
      audioSpecificConfig[i++] = (remainingBit<<7)|((nextByte&0xFE)>>1);
      remainingBit = nextByte&1;
    } while (parseSuccess);
    if (i != ascSize) break; // part of the remaining string was bad

    audioSpecificConfigSize = ascSize;
    return True; // parsing succeeded
  } while (0);

  delete[] audioSpecificConfig;
  return False; // parsing failed
}

unsigned char* parseStreamMuxConfigStr(char const* configStr,
				       // result parameter:
				       unsigned& audioSpecificConfigSize) {
  Boolean audioMuxVersion, allStreamsSameTimeFraming;
  unsigned char numSubFrames, numProgram, numLayer;
  unsigned char* audioSpecificConfig;

  if (!parseStreamMuxConfigStr(configStr,
			       audioMuxVersion, allStreamsSameTimeFraming,
			       numSubFrames, numProgram, numLayer,
			       audioSpecificConfig, audioSpecificConfigSize)) {
    audioSpecificConfigSize = 0;
    return NULL;
  }

  return audioSpecificConfig;
}

unsigned char* parseGeneralConfigStr(char const* configStr,
				     // result parameter:
				     unsigned& configSize) {
  unsigned char* config = NULL;
  do {
    if (configStr == NULL) break;
    configSize = (strlen(configStr)+1)/2;

    config = new unsigned char[configSize];
    if (config == NULL) break;

    unsigned i;
    for (i = 0; i < configSize; ++i) {
      if (!getByte(configStr, config[i])) break;
    }
    if (i != configSize) break; // part of the string was bad

    return config;
  } while (0);

  configSize = 0;
  delete[] config;
  return NULL;
}
