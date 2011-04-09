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
// Demultiplexer for a MPEG 1 or 2 Program Stream
// Implementation

#include "MPEG1or2Demux.hh"
#include "MPEG1or2DemuxedElementaryStream.hh"
#include "StreamParser.hh"
#include <stdlib.h>

////////// MPEGProgramStreamParser definition //////////

// An enum representing the current state of the parser:
enum MPEGParseState {
  PARSING_PACK_HEADER,
  PARSING_SYSTEM_HEADER,
  PARSING_PES_PACKET
};

class MPEGProgramStreamParser: public StreamParser {
public:
  MPEGProgramStreamParser(MPEG1or2Demux* usingSource, FramedSource* inputSource);
  virtual ~MPEGProgramStreamParser();

public:
  unsigned char parse();
      // returns the stream id of a stream for which a frame was acquired,
      // or 0 if no such frame was acquired.

private:
  void setParseState(MPEGParseState parseState);

  void parsePackHeader();
  void parseSystemHeader();
  unsigned char parsePESPacket(); // returns as does parse()

  Boolean isSpecialStreamId(unsigned char stream_id) const;
  // for PES packet header parsing

private:
  MPEG1or2Demux* fUsingSource;
  MPEGParseState fCurrentParseState;
};


////////// MPEG1or2Demux::OutputDescriptor::SavedData definition/implementation //////////

class MPEG1or2Demux::OutputDescriptor::SavedData {
public:
  SavedData(unsigned char* buf, unsigned size)
    : next(NULL), data(buf), dataSize(size), numBytesUsed(0) {
  }
  virtual ~SavedData() {
    delete[] data;
    delete next;
  }

  SavedData* next;
  unsigned char* data;
  unsigned dataSize, numBytesUsed;
};


////////// MPEG1or2Demux implementation //////////

MPEG1or2Demux
::MPEG1or2Demux(UsageEnvironment& env,
		FramedSource* inputSource, Boolean reclaimWhenLastESDies)
  : Medium(env),
    fInputSource(inputSource), fMPEGversion(0),
    fNextAudioStreamNumber(0), fNextVideoStreamNumber(0),
    fReclaimWhenLastESDies(reclaimWhenLastESDies), fNumOutstandingESs(0),
    fNumPendingReads(0), fHaveUndeliveredData(False) {
  fParser = new MPEGProgramStreamParser(this, inputSource);
  for (unsigned i = 0; i < 256; ++i) {
    fOutput[i].savedDataHead = fOutput[i].savedDataTail = NULL;
    fOutput[i].isPotentiallyReadable = False;
    fOutput[i].isCurrentlyActive = False;
    fOutput[i].isCurrentlyAwaitingData = False;
  }
}

MPEG1or2Demux::~MPEG1or2Demux() {
  delete fParser;
  for (unsigned i = 0; i < 256; ++i) delete fOutput[i].savedDataHead;
  Medium::close(fInputSource);
}

MPEG1or2Demux* MPEG1or2Demux
::createNew(UsageEnvironment& env,
	    FramedSource* inputSource, Boolean reclaimWhenLastESDies) {
  // Need to add source type checking here???  #####

  return new MPEG1or2Demux(env, inputSource, reclaimWhenLastESDies);
}

MPEG1or2Demux::SCR::SCR()
  : highBit(0), remainingBits(0), extension(0), isValid(False) {
}

void MPEG1or2Demux
::noteElementaryStreamDeletion(MPEG1or2DemuxedElementaryStream* /*es*/) {
  if (--fNumOutstandingESs == 0 && fReclaimWhenLastESDies) {
    Medium::close(this);
  }
}

void MPEG1or2Demux::flushInput() {
  fParser->flushInput();
}

MPEG1or2DemuxedElementaryStream*
MPEG1or2Demux::newElementaryStream(u_int8_t streamIdTag) {
  ++fNumOutstandingESs;
  fOutput[streamIdTag].isPotentiallyReadable = True;
  return new MPEG1or2DemuxedElementaryStream(envir(), streamIdTag, *this);
}

MPEG1or2DemuxedElementaryStream* MPEG1or2Demux::newAudioStream() {
  unsigned char newAudioStreamTag = 0xC0 | (fNextAudioStreamNumber++&~0xE0);
      // MPEG audio stream tags are 110x xxxx (binary)
  return newElementaryStream(newAudioStreamTag);
}

MPEG1or2DemuxedElementaryStream* MPEG1or2Demux::newVideoStream() {
  unsigned char newVideoStreamTag = 0xE0 | (fNextVideoStreamNumber++&~0xF0);
      // MPEG video stream tags are 1110 xxxx (binary)
  return newElementaryStream(newVideoStreamTag);
}

// Appropriate one of the reserved stream id tags to mean: return raw PES packets:
#define RAW_PES 0xFC

MPEG1or2DemuxedElementaryStream* MPEG1or2Demux::newRawPESStream() {
  return newElementaryStream(RAW_PES);
}

void MPEG1or2Demux::registerReadInterest(u_int8_t streamIdTag,
				     unsigned char* to, unsigned maxSize,
				     FramedSource::afterGettingFunc* afterGettingFunc,
				     void* afterGettingClientData,
				     FramedSource::onCloseFunc* onCloseFunc,
				     void* onCloseClientData) {
  struct OutputDescriptor& out = fOutput[streamIdTag];

  // Make sure this stream is not already being read:
  if (out.isCurrentlyAwaitingData) {
    envir() << "MPEG1or2Demux::registerReadInterest(): attempt to read stream id "
	    << (void*)streamIdTag << " more than once!\n";
    abort();
  }

  out.to = to; out.maxSize = maxSize;
  out.fAfterGettingFunc = afterGettingFunc;
  out.afterGettingClientData = afterGettingClientData;
  out.fOnCloseFunc = onCloseFunc;
  out.onCloseClientData = onCloseClientData;
  out.isCurrentlyActive = True;
  out.isCurrentlyAwaitingData = True;
  // out.frameSize and out.presentationTime will be set when a frame's read

  ++fNumPendingReads;
}

Boolean MPEG1or2Demux::useSavedData(u_int8_t streamIdTag,
				    unsigned char* to, unsigned maxSize,
				    FramedSource::afterGettingFunc* afterGettingFunc,
				    void* afterGettingClientData) {
  struct OutputDescriptor& out = fOutput[streamIdTag];
  if (out.savedDataHead == NULL) return False; // common case

  unsigned totNumBytesCopied = 0;
  while (maxSize > 0 && out.savedDataHead != NULL) {
    OutputDescriptor::SavedData& savedData = *(out.savedDataHead);
    unsigned char* from = &savedData.data[savedData.numBytesUsed];
    unsigned numBytesToCopy = savedData.dataSize - savedData.numBytesUsed;
    if (numBytesToCopy > maxSize) numBytesToCopy = maxSize;
    memmove(to, from, numBytesToCopy);
    to += numBytesToCopy;
    maxSize -= numBytesToCopy;
    out.savedDataTotalSize -= numBytesToCopy;
    totNumBytesCopied += numBytesToCopy;
    savedData.numBytesUsed += numBytesToCopy;
    if (savedData.numBytesUsed == savedData.dataSize) {
      out.savedDataHead = savedData.next;
      if (out.savedDataHead == NULL) out.savedDataTail = NULL;
      savedData.next = NULL;
      delete &savedData;
    }
  }

  out.isCurrentlyActive = True;
  if (afterGettingFunc != NULL) {
    struct timeval presentationTime;
    presentationTime.tv_sec = 0; presentationTime.tv_usec = 0; // should fix #####
    (*afterGettingFunc)(afterGettingClientData, totNumBytesCopied,
			0 /* numTruncatedBytes */, presentationTime,
			0 /* durationInMicroseconds ?????#####*/);
  }
  return True;
}

void MPEG1or2Demux
::continueReadProcessing(void* clientData,
			 unsigned char* /*ptr*/, unsigned /*size*/,
			 struct timeval /*presentationTime*/) {
  MPEG1or2Demux* demux = (MPEG1or2Demux*)clientData;
  demux->continueReadProcessing();
}

void MPEG1or2Demux::continueReadProcessing() {
  while (fNumPendingReads > 0) {
    unsigned char acquiredStreamIdTag = fParser->parse();

    if (acquiredStreamIdTag != 0) {
      // We were able to acquire a frame from the input.
      struct OutputDescriptor& newOut = fOutput[acquiredStreamIdTag];
      newOut.isCurrentlyAwaitingData = False;
        // indicates that we can be read again
        // (This needs to be set before the 'after getting' call below,
        //  in case it tries to read another frame)

      // Call our own 'after getting' function.  Because we're not a 'leaf'
      // source, we can call this directly, without risking infinite recursion.
      if (newOut.fAfterGettingFunc != NULL) {
	(*newOut.fAfterGettingFunc)(newOut.afterGettingClientData,
				    newOut.frameSize, 0 /* numTruncatedBytes */,
				    newOut.presentationTime,
				    0 /* durationInMicroseconds ?????#####*/);
      --fNumPendingReads;
      }
    } else {
      // We were unable to parse a complete frame from the input, because:
      // - we had to read more data from the source stream, or
      // - we found a frame for a stream that was being read, but whose
      //   reader is not ready to get the frame right now, or
      // - the source stream has ended.
      break;
    }
  }
}

void MPEG1or2Demux::getNextFrame(u_int8_t streamIdTag,
				 unsigned char* to, unsigned maxSize,
				 FramedSource::afterGettingFunc* afterGettingFunc,
				 void* afterGettingClientData,
				 FramedSource::onCloseFunc* onCloseFunc,
				 void* onCloseClientData) {
  // First, check whether we have saved data for this stream id:
  if (useSavedData(streamIdTag, to, maxSize,
		   afterGettingFunc, afterGettingClientData)) {
    return;
  }

  // Then save the parameters of the specified stream id:
  registerReadInterest(streamIdTag, to, maxSize,
		       afterGettingFunc, afterGettingClientData,
		       onCloseFunc, onCloseClientData);

  // Next, if we're the only currently pending read, continue looking for data:
  if (fNumPendingReads == 1 || fHaveUndeliveredData) {
    fHaveUndeliveredData = 0;
    continueReadProcessing();
  } // otherwise the continued read processing has already been taken care of
}

void MPEG1or2Demux::stopGettingFrames(u_int8_t streamIdTag) {
    struct OutputDescriptor& out = fOutput[streamIdTag];
    out.isCurrentlyActive = out.isCurrentlyAwaitingData = False;
}

void MPEG1or2Demux::handleClosure(void* clientData) {
  MPEG1or2Demux* demux = (MPEG1or2Demux*)clientData;

  demux->fNumPendingReads = 0;

  // Tell all pending readers that our source has closed.
  // Note that we need to make a copy of our readers' close functions
  // (etc.) before we start calling any of them, in case one of them
  // ends up deleting this.
  struct {
    FramedSource::onCloseFunc* fOnCloseFunc;
    void* onCloseClientData;
  } savedPending[256];
  unsigned i, numPending = 0;
  for (i = 0; i < 256; ++i) {
    struct OutputDescriptor& out = demux->fOutput[i];
    if (out.isCurrentlyAwaitingData) {
      if (out.fOnCloseFunc != NULL) {
	savedPending[numPending].fOnCloseFunc = out.fOnCloseFunc;
	savedPending[numPending].onCloseClientData = out.onCloseClientData;
	++numPending;
      }
    }
    delete out.savedDataHead; out.savedDataHead = out.savedDataTail = NULL;
    out.savedDataTotalSize = 0;
    out.isPotentiallyReadable = out.isCurrentlyActive = out.isCurrentlyAwaitingData
      = False;
  }
  for (i = 0; i < numPending; ++i) {
    (*savedPending[i].fOnCloseFunc)(savedPending[i].onCloseClientData);
  }
}


////////// MPEGProgramStreamParser implementation //////////

#include <string.h>

MPEGProgramStreamParser::MPEGProgramStreamParser(MPEG1or2Demux* usingSource,
						 FramedSource* inputSource)
  : StreamParser(inputSource, MPEG1or2Demux::handleClosure, usingSource,
		 &MPEG1or2Demux::continueReadProcessing, usingSource),
  fUsingSource(usingSource), fCurrentParseState(PARSING_PACK_HEADER) {
}

MPEGProgramStreamParser::~MPEGProgramStreamParser() {
}

void MPEGProgramStreamParser::setParseState(MPEGParseState parseState) {
  fCurrentParseState = parseState;
  saveParserState();
}

unsigned char MPEGProgramStreamParser::parse() {
  unsigned char acquiredStreamTagId = 0;

  try {
    do {
      switch (fCurrentParseState) {
      case PARSING_PACK_HEADER: {
	parsePackHeader();
	break;
      }
      case PARSING_SYSTEM_HEADER: {
	parseSystemHeader();
	break;
      }
      case PARSING_PES_PACKET: {
	acquiredStreamTagId = parsePESPacket();
	break;
      }
      }
    } while(acquiredStreamTagId == 0);

    return acquiredStreamTagId;
  } catch (int /*e*/) {
#ifdef DEBUG
    fprintf(stderr, "MPEGProgramStreamParser::parse() EXCEPTION (This is normal behavior - *not* an error)\n");
    fflush(stderr);
#endif
    return 0;  // the parsing got interrupted
  }
}

#define PACK_START_CODE          0x000001BA
#define SYSTEM_HEADER_START_CODE 0x000001BB
#define PACKET_START_CODE_PREFIX 0x00000100

static inline Boolean isPacketStartCode(unsigned code) {
  return (code&0xFFFFFF00) == PACKET_START_CODE_PREFIX
    && code > SYSTEM_HEADER_START_CODE;
}

void MPEGProgramStreamParser::parsePackHeader() {
#ifdef DEBUG
  fprintf(stderr, "parsing pack header\n"); fflush(stderr);
#endif
  unsigned first4Bytes;
  while (1) {
    first4Bytes = test4Bytes();

    // We're supposed to have a pack header here, but check also for
    // a system header or a PES packet, just in case:
    if (first4Bytes == PACK_START_CODE) {
      skipBytes(4);
      break;
    } else if (first4Bytes == SYSTEM_HEADER_START_CODE) {
#ifdef DEBUG
      fprintf(stderr, "found system header instead of pack header\n");
#endif
      setParseState(PARSING_SYSTEM_HEADER);
      return;
    } else if (isPacketStartCode(first4Bytes)) {
#ifdef DEBUG
      fprintf(stderr, "found packet start code 0x%02x instead of pack header\n", first4Bytes);
#endif
      setParseState(PARSING_PES_PACKET);
      return;
    }

    setParseState(PARSING_PACK_HEADER); // ensures we progress over bad data
    if ((first4Bytes&0xFF) > 1) { // a system code definitely doesn't start here
      skipBytes(4);
    } else {
      skipBytes(1);
    }
  }

  // The size of the pack header differs depending on whether it's
  // MPEG-1 or MPEG-2.  The next byte tells us this:
  unsigned char nextByte = get1Byte();
  MPEG1or2Demux::SCR& scr = fUsingSource->fLastSeenSCR; // alias
  if ((nextByte&0xF0) == 0x20) { // MPEG-1
    fUsingSource->fMPEGversion = 1;
    scr.highBit =  (nextByte&0x08)>>3;
    scr.remainingBits = (nextByte&0x06)<<29;
    unsigned next4Bytes = get4Bytes();
    scr.remainingBits |= (next4Bytes&0xFFFE0000)>>2;
    scr.remainingBits |= (next4Bytes&0x0000FFFE)>>1;
    scr.extension = 0;
    scr.isValid = True;
    skipBits(24);

#if defined(DEBUG_TIMESTAMPS) || defined(DEBUG_SCR_TIMESTAMPS)
    fprintf(stderr, "pack hdr system_clock_reference_base: 0x%x",
	    scr.highBit);
    fprintf(stderr, "%08x\n", scr.remainingBits);
#endif
  } else if ((nextByte&0xC0) == 0x40) { // MPEG-2
    fUsingSource->fMPEGversion = 2;
    scr.highBit =  (nextByte&0x20)>>5;
    scr.remainingBits = (nextByte&0x18)<<27;
    scr.remainingBits |= (nextByte&0x03)<<28;
    unsigned next4Bytes = get4Bytes();
    scr.remainingBits |= (next4Bytes&0xFFF80000)>>4;
    scr.remainingBits |= (next4Bytes&0x0003FFF8)>>3;
    scr.extension = (next4Bytes&0x00000003)<<7;
    next4Bytes = get4Bytes();
    scr.extension |= (next4Bytes&0xFE000000)>>25;
    scr.isValid = True;
    skipBits(5);

#if defined(DEBUG_TIMESTAMPS) || defined(DEBUG_SCR_TIMESTAMPS)
    fprintf(stderr, "pack hdr system_clock_reference_base: 0x%x",
	    scr.highBit);
    fprintf(stderr, "%08x\n", scr.remainingBits);
    fprintf(stderr, "pack hdr system_clock_reference_extension: 0x%03x\n",
	    scr.extension);
#endif
    unsigned char pack_stuffing_length = getBits(3);
    skipBytes(pack_stuffing_length);
  } else { // unknown
    fUsingSource->envir() << "StreamParser::parsePack() saw strange byte "
			  << (void*)nextByte
			  << " following pack_start_code\n";
  }

  // Check for a System Header next:
  setParseState(PARSING_SYSTEM_HEADER);
}

void MPEGProgramStreamParser::parseSystemHeader() {
#ifdef DEBUG
  fprintf(stderr, "parsing system header\n"); fflush(stderr);
#endif
  unsigned next4Bytes = test4Bytes();
  if (next4Bytes != SYSTEM_HEADER_START_CODE) {
    // The system header was optional.  Look for a PES Packet instead:
    setParseState(PARSING_PES_PACKET);
    return;
  }

#ifdef DEBUG
  fprintf(stderr, "saw system_header_start_code\n"); fflush(stderr);
#endif
  skipBytes(4); // we've already seen the system_header_start_code

  unsigned short remaining_header_length = get2Bytes();

  // According to the MPEG-1 and MPEG-2 specs, "remaining_header_length" should be
  // at least 6 bytes.  Check this now:
  if (remaining_header_length < 6) {
    fUsingSource->envir() << "StreamParser::parseSystemHeader(): saw strange header_length: "
			  << remaining_header_length << " < 6\n";
  }
  skipBytes(remaining_header_length);

  // Check for a PES Packet next:
  setParseState(PARSING_PES_PACKET);
}

#define private_stream_1 0xBD
#define private_stream_2 0xBF

// A test for stream ids that are exempt from normal PES packet header parsing
Boolean MPEGProgramStreamParser
::isSpecialStreamId(unsigned char stream_id) const {
  if (stream_id == RAW_PES) return True; // hack

  if (fUsingSource->fMPEGversion == 1) {
    return stream_id == private_stream_2;
  } else { // assume MPEG-2
    if (stream_id <= private_stream_2) {
      return stream_id != private_stream_1;
    } else if ((stream_id&0xF0) == 0xF0) {
      unsigned char lower4Bits = stream_id&0x0F;
      return lower4Bits <= 2 || lower4Bits == 0x8 || lower4Bits == 0xF;
    } else {
      return False;
    }
  }
}

#define READER_NOT_READY 2

unsigned char MPEGProgramStreamParser::parsePESPacket() {
#ifdef DEBUG
  fprintf(stderr, "parsing PES packet\n"); fflush(stderr);
#endif
  unsigned next4Bytes = test4Bytes();
  if (!isPacketStartCode(next4Bytes)) {
    // The PES Packet was optional.  Look for a Pack Header instead:
    setParseState(PARSING_PACK_HEADER);
    return 0;
  }

#ifdef DEBUG
  fprintf(stderr, "saw packet_start_code_prefix\n"); fflush(stderr);
#endif
  skipBytes(3); // we've already seen the packet_start_code_prefix

  unsigned char stream_id = get1Byte();
#if defined(DEBUG) || defined(DEBUG_TIMESTAMPS)
  unsigned char streamNum = stream_id;
  char const* streamTypeStr;
  if ((stream_id&0xE0) == 0xC0) {
    streamTypeStr = "audio";
    streamNum = stream_id&~0xE0;
  } else if ((stream_id&0xF0) == 0xE0) {
    streamTypeStr = "video";
    streamNum = stream_id&~0xF0;
  } else if (stream_id == 0xbc) {
    streamTypeStr = "reserved";
  } else if (stream_id == 0xbd) {
    streamTypeStr = "private_1";
  } else if (stream_id == 0xbe) {
    streamTypeStr = "padding";
  } else if (stream_id == 0xbf) {
    streamTypeStr = "private_2";
  } else {
    streamTypeStr = "unknown";
  }
#endif
#ifdef DEBUG
  static unsigned frameCount = 1;
  fprintf(stderr, "%d, saw %s stream: 0x%02x\n", frameCount, streamTypeStr, streamNum); fflush(stderr);
#endif

  unsigned short PES_packet_length = get2Bytes();
#ifdef DEBUG
  fprintf(stderr, "PES_packet_length: %d\n", PES_packet_length); fflush(stderr);
#endif

  // Parse over the rest of the header, until we get to the packet data itself.
  // This varies depending upon the MPEG version:
  if (fUsingSource->fOutput[RAW_PES].isPotentiallyReadable) {
    // Hack: We've been asked to return raw PES packets, for every stream:
    stream_id = RAW_PES;
  }
  unsigned savedParserOffset = curOffset();
#ifdef DEBUG_TIMESTAMPS
  unsigned char pts_highBit = 0;
  unsigned pts_remainingBits = 0;
  unsigned char dts_highBit = 0;
  unsigned dts_remainingBits = 0;
#endif
  if (fUsingSource->fMPEGversion == 1) {
    if (!isSpecialStreamId(stream_id)) {
      unsigned char nextByte;
      while ((nextByte = get1Byte()) == 0xFF) { // stuffing_byte
      }
      if ((nextByte&0xC0) == 0x40) { // '01'
	skipBytes(1);
	nextByte = get1Byte();
      }
      if ((nextByte&0xF0) == 0x20) { // '0010'
#ifdef DEBUG_TIMESTAMPS
	pts_highBit =  (nextByte&0x08)>>3;
	pts_remainingBits = (nextByte&0x06)<<29;
	unsigned next4Bytes = get4Bytes();
	pts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	pts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;
#else
	skipBytes(4);
#endif
      } else if ((nextByte&0xF0) == 0x30) { // '0011'
#ifdef DEBUG_TIMESTAMPS
	pts_highBit =  (nextByte&0x08)>>3;
	pts_remainingBits = (nextByte&0x06)<<29;
	unsigned next4Bytes = get4Bytes();
	pts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	pts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;

	nextByte = get1Byte();
	dts_highBit =  (nextByte&0x08)>>3;
	dts_remainingBits = (nextByte&0x06)<<29;
	next4Bytes = get4Bytes();
	dts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	dts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;
#else
	skipBytes(9);
#endif
      }
    }
  } else { // assume MPEG-2
    if (!isSpecialStreamId(stream_id)) {
      // Fields in the next 3 bytes determine the size of the rest:
      unsigned next3Bytes = getBits(24);
#ifdef DEBUG_TIMESTAMPS
      unsigned char PTS_DTS_flags       = (next3Bytes&0x00C000)>>14;
#endif
#ifdef undef
      unsigned char ESCR_flag           = (next3Bytes&0x002000)>>13;
      unsigned char ES_rate_flag        = (next3Bytes&0x001000)>>12;
      unsigned char DSM_trick_mode_flag = (next3Bytes&0x000800)>>11;
#endif
      unsigned char PES_header_data_length = (next3Bytes&0x0000FF);
#ifdef DEBUG
      fprintf(stderr, "PES_header_data_length: 0x%02x\n", PES_header_data_length); fflush(stderr);
#endif
#ifdef DEBUG_TIMESTAMPS
      if (PTS_DTS_flags == 0x2 && PES_header_data_length >= 5) {
	unsigned char nextByte = get1Byte();
	pts_highBit =  (nextByte&0x08)>>3;
	pts_remainingBits = (nextByte&0x06)<<29;
	unsigned next4Bytes = get4Bytes();
	pts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	pts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;

	skipBytes(PES_header_data_length-5);
      } else if (PTS_DTS_flags == 0x3 && PES_header_data_length >= 10) {
	unsigned char nextByte = get1Byte();
	pts_highBit =  (nextByte&0x08)>>3;
	pts_remainingBits = (nextByte&0x06)<<29;
	unsigned next4Bytes = get4Bytes();
	pts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	pts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;

	nextByte = get1Byte();
	dts_highBit =  (nextByte&0x08)>>3;
	dts_remainingBits = (nextByte&0x06)<<29;
	next4Bytes = get4Bytes();
	dts_remainingBits |= (next4Bytes&0xFFFE0000)>>2;
	dts_remainingBits |= (next4Bytes&0x0000FFFE)>>1;

	skipBytes(PES_header_data_length-10);
      }
#else
      skipBytes(PES_header_data_length);
#endif
    }
  }
#ifdef DEBUG_TIMESTAMPS
  fprintf(stderr, "%s stream, ", streamTypeStr);
  fprintf(stderr, "packet presentation_time_stamp: 0x%x", pts_highBit);
  fprintf(stderr, "%08x\n", pts_remainingBits);
  fprintf(stderr, "\t\tpacket decoding_time_stamp: 0x%x", dts_highBit);
  fprintf(stderr, "%08x\n", dts_remainingBits);
#endif

  // The rest of the packet will be the "PES_packet_data_byte"s
  // Make sure that "PES_packet_length" was consistent with where we are now:
  unsigned char acquiredStreamIdTag = 0;
  unsigned currentParserOffset = curOffset();
  unsigned bytesSkipped = currentParserOffset - savedParserOffset;
  if (stream_id == RAW_PES) {
    restoreSavedParserState(); // so we deliver from the beginning of the PES packet
    PES_packet_length += 6; // to include the whole of the PES packet
    bytesSkipped = 0;
  }
  if (PES_packet_length < bytesSkipped) {
    fUsingSource->envir() << "StreamParser::parsePESPacket(): saw inconsistent PES_packet_length "
			  << PES_packet_length << " < "
			  << bytesSkipped << "\n";
  } else {
    PES_packet_length -= bytesSkipped;
#ifdef DEBUG
    unsigned next4Bytes = test4Bytes();
#endif

    // Check whether our using source is interested in this stream type.
    // If so, deliver the frame to him:
    MPEG1or2Demux::OutputDescriptor_t& out = fUsingSource->fOutput[stream_id];
    if (out.isCurrentlyAwaitingData) {
      unsigned numBytesToCopy;
      if (PES_packet_length > out.maxSize) {
	fUsingSource->envir() << "MPEGProgramStreamParser::parsePESPacket() error: PES_packet_length ("
			      << PES_packet_length
			      << ") exceeds max frame size asked for ("
			      << out.maxSize << ")\n";
	numBytesToCopy = out.maxSize;
      } else {
	numBytesToCopy = PES_packet_length;
      }

      getBytes(out.to, numBytesToCopy);
      out.frameSize = numBytesToCopy;
#ifdef DEBUG
      fprintf(stderr, "%d, %d bytes of PES_packet_data (out.maxSize: %d); first 4 bytes: 0x%08x\n", frameCount, numBytesToCopy, out.maxSize, next4Bytes); fflush(stderr);
#endif
      // set out.presentationTime later #####
      acquiredStreamIdTag = stream_id;
      PES_packet_length -= numBytesToCopy;
    } else if (out.isCurrentlyActive) {
      // Someone has been reading this stream, but isn't right now.
      // We can't deliver this frame until he asks for it, so punt for now.
      // The next time he asks for a frame, he'll get it.
#ifdef DEBUG
      fprintf(stderr, "%d, currently undeliverable PES data; first 4 bytes: 0x%08x - currently undeliverable!\n", frameCount, next4Bytes); fflush(stderr);
#endif
      restoreSavedParserState(); // so we read from the beginning next time
      fUsingSource->fHaveUndeliveredData = True;
      throw READER_NOT_READY;
    } else if (out.isPotentiallyReadable &&
	       out.savedDataTotalSize + PES_packet_length < 1000000 /*limit*/) {
      // Someone is interested in this stream, but hasn't begun reading it yet.
      // Save this data, so that the reader will get it when he later asks for it.
      unsigned char* buf = new unsigned char[PES_packet_length];
      getBytes(buf, PES_packet_length);
      MPEG1or2Demux::OutputDescriptor::SavedData* savedData
	= new MPEG1or2Demux::OutputDescriptor::SavedData(buf, PES_packet_length);
      if (out.savedDataHead == NULL) {
	out.savedDataHead = out.savedDataTail = savedData;
      } else {
	out.savedDataTail->next = savedData;
	out.savedDataTail = savedData;
      }
      out.savedDataTotalSize += PES_packet_length;
      PES_packet_length = 0;
    }
    skipBytes(PES_packet_length);
  }

  // Check for another PES Packet next:
  setParseState(PARSING_PES_PACKET);
#ifdef DEBUG
  ++frameCount;
#endif
 return acquiredStreamIdTag;
}
