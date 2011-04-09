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
// A filter that breaks up an MPEG-4 video elementary stream into
//   frames for:
// - Visual Object Sequence (VS) Header + Visual Object (VO) Header
//   + Video Object Layer (VOL) Header
// - Group of VOP (GOV) Header
// - VOP frame
// Implementation

#include "MPEG4VideoStreamFramer.hh"
#include "MPEGVideoStreamParser.hh"
#include <string.h>

////////// MPEG4VideoStreamParser definition //////////

// An enum representing the current state of the parser:
enum MPEGParseState {
  PARSING_VISUAL_OBJECT_SEQUENCE,
  PARSING_VISUAL_OBJECT_SEQUENCE_SEEN_CODE,
  PARSING_VISUAL_OBJECT,
  PARSING_VIDEO_OBJECT_LAYER,
  PARSING_GROUP_OF_VIDEO_OBJECT_PLANE,
  PARSING_VIDEO_OBJECT_PLANE,
  PARSING_VISUAL_OBJECT_SEQUENCE_END_CODE
};

class MPEG4VideoStreamParser: public MPEGVideoStreamParser {
public:
  MPEG4VideoStreamParser(MPEG4VideoStreamFramer* usingSource,
			 FramedSource* inputSource);
  virtual ~MPEG4VideoStreamParser();

private: // redefined virtual functions:
  virtual void flushInput();
  virtual unsigned parse();

private:
  MPEG4VideoStreamFramer* usingSource() {
    return (MPEG4VideoStreamFramer*)fUsingSource;
  }
  void setParseState(MPEGParseState parseState);

  unsigned parseVisualObjectSequence(Boolean haveSeenStartCode = False);
  unsigned parseVisualObject();
  unsigned parseVideoObjectLayer();
  unsigned parseGroupOfVideoObjectPlane();
  unsigned parseVideoObjectPlane();
  unsigned parseVisualObjectSequenceEndCode();

  // These are used for parsing within an already-read frame:
  Boolean getNextFrameBit(u_int8_t& result);
  Boolean getNextFrameBits(unsigned numBits, u_int32_t& result);

  // Which are used by:
  void analyzeVOLHeader();

private:
  MPEGParseState fCurrentParseState;
  unsigned fNumBitsSeenSoFar; // used by the getNextFrameBit*() routines
  u_int32_t vop_time_increment_resolution;
  unsigned fNumVTIRBits;
      // # of bits needed to count to "vop_time_increment_resolution"
  u_int8_t fixed_vop_rate;
  unsigned fixed_vop_time_increment; // used if 'fixed_vop_rate' is set
  unsigned fSecondsSinceLastTimeCode, fTotalTicksSinceLastTimeCode, fPrevNewTotalTicks;
  unsigned fPrevPictureCountDelta;
  Boolean fJustSawTimeCode;
};


////////// MPEG4VideoStreamFramer implementation //////////

MPEG4VideoStreamFramer*
MPEG4VideoStreamFramer::createNew(UsageEnvironment& env,
				  FramedSource* inputSource) {
  // Need to add source type checking here???  #####
  return new MPEG4VideoStreamFramer(env, inputSource);
}

unsigned char* MPEG4VideoStreamFramer
::getConfigBytes(unsigned& numBytes) const {
  numBytes = fNumConfigBytes;
  return fConfigBytes;
}

MPEG4VideoStreamFramer::MPEG4VideoStreamFramer(UsageEnvironment& env,
					       FramedSource* inputSource,
					       Boolean createParser)
  : MPEGVideoStreamFramer(env, inputSource),
    fProfileAndLevelIndication(0),
    fConfigBytes(NULL), fNumConfigBytes(0),
    fNewConfigBytes(NULL), fNumNewConfigBytes(0) {
  fParser = createParser
    ? new MPEG4VideoStreamParser(this, inputSource)
    : NULL;
}

MPEG4VideoStreamFramer::~MPEG4VideoStreamFramer() {
  delete[] fConfigBytes; delete[] fNewConfigBytes;
}

void MPEG4VideoStreamFramer::startNewConfig() {
  delete[] fNewConfigBytes; fNewConfigBytes = NULL;
  fNumNewConfigBytes = 0;
}

void MPEG4VideoStreamFramer
::appendToNewConfig(unsigned char* newConfigBytes, unsigned numNewBytes) {
  // Allocate a new block of memory for the new config bytes:
  unsigned char* configNew
    = new unsigned char[fNumNewConfigBytes + numNewBytes];

  // Copy the old, then the new, config bytes there:
  memmove(configNew, fNewConfigBytes, fNumNewConfigBytes);
  memmove(&configNew[fNumNewConfigBytes], newConfigBytes, numNewBytes);

  delete[] fNewConfigBytes; fNewConfigBytes = configNew;
  fNumNewConfigBytes += numNewBytes;
}

void MPEG4VideoStreamFramer::completeNewConfig() {
  delete[] fConfigBytes; fConfigBytes = fNewConfigBytes;
  fNewConfigBytes = NULL;
  fNumConfigBytes = fNumNewConfigBytes;
  fNumNewConfigBytes = 0;
}

Boolean MPEG4VideoStreamFramer::isMPEG4VideoStreamFramer() const {
  return True;
}

////////// MPEG4VideoStreamParser implementation //////////

MPEG4VideoStreamParser
::MPEG4VideoStreamParser(MPEG4VideoStreamFramer* usingSource,
			 FramedSource* inputSource)
  : MPEGVideoStreamParser(usingSource, inputSource),
    fCurrentParseState(PARSING_VISUAL_OBJECT_SEQUENCE),
    vop_time_increment_resolution(0), fNumVTIRBits(0),
    fixed_vop_rate(0), fixed_vop_time_increment(0),
    fSecondsSinceLastTimeCode(0), fTotalTicksSinceLastTimeCode(0),
    fPrevNewTotalTicks(0), fPrevPictureCountDelta(1), fJustSawTimeCode(False) {
}

MPEG4VideoStreamParser::~MPEG4VideoStreamParser() {
}

void MPEG4VideoStreamParser::setParseState(MPEGParseState parseState) {
  fCurrentParseState = parseState;
  MPEGVideoStreamParser::setParseState();
}

void MPEG4VideoStreamParser::flushInput() {
  fSecondsSinceLastTimeCode = 0;
  fTotalTicksSinceLastTimeCode = 0;
  fPrevNewTotalTicks = 0;
  fPrevPictureCountDelta = 1;

  StreamParser::flushInput();
  if (fCurrentParseState != PARSING_VISUAL_OBJECT_SEQUENCE) {
    setParseState(PARSING_VISUAL_OBJECT_SEQUENCE); // later, change to GOV or VOP? #####
  }
}


unsigned MPEG4VideoStreamParser::parse() {
  try {
    switch (fCurrentParseState) {
    case PARSING_VISUAL_OBJECT_SEQUENCE: {
      return parseVisualObjectSequence();
    }
    case PARSING_VISUAL_OBJECT_SEQUENCE_SEEN_CODE: {
      return parseVisualObjectSequence(True);
    }
    case PARSING_VISUAL_OBJECT: {
      return parseVisualObject();
    }
    case PARSING_VIDEO_OBJECT_LAYER: {
      return parseVideoObjectLayer();
    }
    case PARSING_GROUP_OF_VIDEO_OBJECT_PLANE: {
      return parseGroupOfVideoObjectPlane();
    }
    case PARSING_VIDEO_OBJECT_PLANE: {
      return parseVideoObjectPlane();
    }
    case PARSING_VISUAL_OBJECT_SEQUENCE_END_CODE: {
      return parseVisualObjectSequenceEndCode();
    }
    default: {
      return 0; // shouldn't happen
    }
    }
  } catch (int /*e*/) {
#ifdef DEBUG
    fprintf(stderr, "MPEG4VideoStreamParser::parse() EXCEPTION (This is normal behavior - *not* an error)\n");
#endif
    return 0;  // the parsing got interrupted
  }
}

#define VISUAL_OBJECT_SEQUENCE_START_CODE 0x000001B0
#define VISUAL_OBJECT_SEQUENCE_END_CODE   0x000001B1
#define GROUP_VOP_START_CODE              0x000001B3
#define VISUAL_OBJECT_START_CODE          0x000001B5
#define VOP_START_CODE                    0x000001B6

unsigned MPEG4VideoStreamParser
::parseVisualObjectSequence(Boolean haveSeenStartCode) {
#ifdef DEBUG
  fprintf(stderr, "parsing VisualObjectSequence\n");
#endif
  usingSource()->startNewConfig();
  u_int32_t first4Bytes;
  if (!haveSeenStartCode) {
    while ((first4Bytes = test4Bytes()) != VISUAL_OBJECT_SEQUENCE_START_CODE) {
#ifdef DEBUG
      fprintf(stderr, "ignoring non VS header: 0x%08x\n", first4Bytes);
#endif
      get1Byte(); setParseState(PARSING_VISUAL_OBJECT_SEQUENCE);
          // ensures we progress over bad data
    }
    first4Bytes = get4Bytes();
  } else {
    // We've already seen the start code
    first4Bytes = VISUAL_OBJECT_SEQUENCE_START_CODE;
  }
  save4Bytes(first4Bytes);

  // The next byte is the "profile_and_level_indication":
  u_int8_t pali = get1Byte();
#ifdef DEBUG
  fprintf(stderr, "profile_and_level_indication: %02x\n", pali);
#endif
  saveByte(pali);
  usingSource()->fProfileAndLevelIndication = pali;

  // Now, copy all bytes that we see, up until we reach
  // a VISUAL_OBJECT_START_CODE:
  u_int32_t next4Bytes = get4Bytes();
  while (next4Bytes != VISUAL_OBJECT_START_CODE) {
    saveToNextCode(next4Bytes);
  }

  setParseState(PARSING_VISUAL_OBJECT);

  // Compute this frame's presentation time:
  usingSource()->computePresentationTime(fTotalTicksSinceLastTimeCode);

  // This header forms part of the 'configuration' information:
  usingSource()->appendToNewConfig(fStartOfFrame, curFrameSize());

  return curFrameSize();
}

static inline Boolean isVideoObjectStartCode(u_int32_t code) {
  return code >= 0x00000100 && code <= 0x0000011F;
}

unsigned MPEG4VideoStreamParser::parseVisualObject() {
#ifdef DEBUG
  fprintf(stderr, "parsing VisualObject\n");
#endif
  // Note that we've already read the VISUAL_OBJECT_START_CODE
  save4Bytes(VISUAL_OBJECT_START_CODE);

  // Next, extract the "visual_object_type" from the next 1 or 2 bytes:
  u_int8_t nextByte = get1Byte(); saveByte(nextByte);
  Boolean is_visual_object_identifier = (nextByte&0x80) != 0;
  u_int8_t visual_object_type;
  if (is_visual_object_identifier) {
#ifdef DEBUG
    fprintf(stderr, "visual_object_verid: 0x%x; visual_object_priority: 0x%x\n", (nextByte&0x78)>>3, (nextByte&0x07));
#endif
    nextByte = get1Byte(); saveByte(nextByte);
    visual_object_type = (nextByte&0xF0)>>4;
  } else {
    visual_object_type = (nextByte&0x78)>>3;
  }
#ifdef DEBUG
  fprintf(stderr, "visual_object_type: 0x%x\n", visual_object_type);
#endif
  // At present, we support only the "Video ID" "visual_object_type" (1)
  if (visual_object_type != 1) {
    usingSource()->envir() << "MPEG4VideoStreamParser::parseVisualObject(): Warning: We don't handle visual_object_type " << visual_object_type << "\n";
  }

  // Now, copy all bytes that we see, up until we reach
  // a video_object_start_code
  u_int32_t next4Bytes = get4Bytes();
  while (!isVideoObjectStartCode(next4Bytes)) {
    saveToNextCode(next4Bytes);
  }
  save4Bytes(next4Bytes);
#ifdef DEBUG
  fprintf(stderr, "saw a video_object_start_code: 0x%08x\n", next4Bytes);
#endif

  setParseState(PARSING_VIDEO_OBJECT_LAYER);

  // Compute this frame's presentation time:
  usingSource()->computePresentationTime(fTotalTicksSinceLastTimeCode);

  // This header forms part of the 'configuration' information:
  usingSource()->appendToNewConfig(fStartOfFrame, curFrameSize());

  return curFrameSize();
}

static inline Boolean isVideoObjectLayerStartCode(u_int32_t code) {
  return code >= 0x00000120 && code <= 0x0000012F;
}

Boolean MPEG4VideoStreamParser::getNextFrameBit(u_int8_t& result) {
  if (fNumBitsSeenSoFar/8 >= curFrameSize()) return False;

  u_int8_t nextByte = fStartOfFrame[fNumBitsSeenSoFar/8];
  result = (nextByte>>(7-fNumBitsSeenSoFar%8))&1;
  ++fNumBitsSeenSoFar;
  return True;
}

Boolean MPEG4VideoStreamParser::getNextFrameBits(unsigned numBits,
						 u_int32_t& result) {
  result = 0;
  for (unsigned i = 0; i < numBits; ++i) {
    u_int8_t nextBit;
    if (!getNextFrameBit(nextBit)) return False;
    result = (result<<1)|nextBit;
  }
  return True;
}

void MPEG4VideoStreamParser::analyzeVOLHeader() {
  // Extract timing information (in particular,
  // "vop_time_increment_resolution") from the VOL Header:
  fNumBitsSeenSoFar = 41;
  do {
    u_int8_t is_object_layer_identifier;
    if (!getNextFrameBit(is_object_layer_identifier)) break;
    if (is_object_layer_identifier) fNumBitsSeenSoFar += 7;

    u_int32_t aspect_ratio_info;
    if (!getNextFrameBits(4, aspect_ratio_info)) break;
    if (aspect_ratio_info == 15 /*extended_PAR*/) fNumBitsSeenSoFar += 16;

    u_int8_t vol_control_parameters;
    if (!getNextFrameBit(vol_control_parameters)) break;
    if (vol_control_parameters) {
      fNumBitsSeenSoFar += 3; // chroma_format; low_delay
      u_int8_t vbw_parameters;
      if (!getNextFrameBit(vbw_parameters)) break;
      if (vbw_parameters) fNumBitsSeenSoFar += 79;
    }

    fNumBitsSeenSoFar += 2; // video_object_layer_shape
    u_int8_t marker_bit;
    if (!getNextFrameBit(marker_bit)) break;
    if (marker_bit != 1) { // sanity check
      usingSource()->envir() << "MPEG4VideoStreamParser::analyzeVOLHeader(): marker_bit 1 not set!\n";
      break;
    }

    if (!getNextFrameBits(16, vop_time_increment_resolution)) break;
#ifdef DEBUG
    fprintf(stderr, "vop_time_increment_resolution: %d\n", vop_time_increment_resolution);
#endif
    if (vop_time_increment_resolution == 0) {
      usingSource()->envir() << "MPEG4VideoStreamParser::analyzeVOLHeader(): vop_time_increment_resolution is zero!\n";
      break;
    }
    // Compute how many bits are necessary to represent this:
    fNumVTIRBits = 0;
    for (unsigned test = vop_time_increment_resolution; test>0; test /= 2) {
      ++fNumVTIRBits;
    }

    if (!getNextFrameBit(marker_bit)) break;
    if (marker_bit != 1) { // sanity check
      usingSource()->envir() << "MPEG4VideoStreamParser::analyzeVOLHeader(): marker_bit 2 not set!\n";
      break;
    }

    if (!getNextFrameBit(fixed_vop_rate)) break;
    if (fixed_vop_rate) {
      // Get the following "fixed_vop_time_increment":
      if (!getNextFrameBits(fNumVTIRBits, fixed_vop_time_increment)) break;
#ifdef DEBUG
      fprintf(stderr, "fixed_vop_time_increment: %d\n", fixed_vop_time_increment);
      if (fixed_vop_time_increment == 0) {
	usingSource()->envir() << "MPEG4VideoStreamParser::analyzeVOLHeader(): fixed_vop_time_increment is zero!\n";
      }
#endif
    }
    // Use "vop_time_increment_resolution" as the 'frame rate'
    // (really, 'tick rate'):
    usingSource()->fFrameRate = (double)vop_time_increment_resolution;
#ifdef DEBUG
    fprintf(stderr, "fixed_vop_rate: %d; 'frame' (really tick) rate: %f\n", fixed_vop_rate, usingSource()->fFrameRate);
#endif

    return;
  } while (0);

  if (fNumBitsSeenSoFar/8 >= curFrameSize()) {
    char errMsg[200];
    sprintf(errMsg, "Not enough bits in VOL header: %d/8 >= %d\n", fNumBitsSeenSoFar, curFrameSize());
    usingSource()->envir() << errMsg;
  }
}

unsigned MPEG4VideoStreamParser::parseVideoObjectLayer() {
#ifdef DEBUG
  fprintf(stderr, "parsing VideoObjectLayer\n");
#endif
  // The first 4 bytes must be a "video_object_layer_start_code".
  // If not, this is a 'short video header', which we currently
  // don't support:
  u_int32_t next4Bytes = get4Bytes();
  if (!isVideoObjectLayerStartCode(next4Bytes)) {
    usingSource()->envir() << "MPEG4VideoStreamParser::parseVideoObjectLayer(): This appears to be a 'short video header', which we current don't support\n";
  }

  // Now, copy all bytes that we see, up until we reach
  // a GROUP_VOP_START_CODE or a VOP_START_CODE:
  do {
    saveToNextCode(next4Bytes);
  } while (next4Bytes != GROUP_VOP_START_CODE
	   && next4Bytes != VOP_START_CODE);

  analyzeVOLHeader();

  setParseState((next4Bytes == GROUP_VOP_START_CODE)
                ? PARSING_GROUP_OF_VIDEO_OBJECT_PLANE
		: PARSING_VIDEO_OBJECT_PLANE);

  // Compute this frame's presentation time:
  usingSource()->computePresentationTime(fTotalTicksSinceLastTimeCode);

  // This header ends the 'configuration' information:
  usingSource()->appendToNewConfig(fStartOfFrame, curFrameSize());
  usingSource()->completeNewConfig();

  return curFrameSize();
}

unsigned MPEG4VideoStreamParser::parseGroupOfVideoObjectPlane() {
#ifdef DEBUG
  fprintf(stderr, "parsing GroupOfVideoObjectPlane\n");
#endif
  // Note that we've already read the GROUP_VOP_START_CODE
  save4Bytes(GROUP_VOP_START_CODE);

  // Next, extract the (18-bit) time code from the next 3 bytes:
  u_int8_t next3Bytes[3];
  getBytes(next3Bytes, 3);
  saveByte(next3Bytes[0]);saveByte(next3Bytes[1]);saveByte(next3Bytes[2]);
  unsigned time_code
    = (next3Bytes[0]<<10)|(next3Bytes[1]<<2)|(next3Bytes[2]>>6);
  unsigned time_code_hours    = (time_code&0x0003E000)>>13;
  unsigned time_code_minutes  = (time_code&0x00001F80)>>7;
#if defined(DEBUG) || defined(DEBUG_TIMESTAMPS)
  Boolean marker_bit          = (time_code&0x00000040) != 0;
#endif
  unsigned time_code_seconds  = (time_code&0x0000003F);
#if defined(DEBUG) || defined(DEBUG_TIMESTAMPS)
  fprintf(stderr, "time_code: 0x%05x, hours %d, minutes %d, marker_bit %d, seconds %d\n", time_code, time_code_hours, time_code_minutes, marker_bit, time_code_seconds);
#endif
  fJustSawTimeCode = True;

  // Now, copy all bytes that we see, up until we reach a VOP_START_CODE:
  u_int32_t next4Bytes = get4Bytes();
  while (next4Bytes != VOP_START_CODE) {
    saveToNextCode(next4Bytes);
  }

  // Compute this frame's presentation time:
  usingSource()->computePresentationTime(fTotalTicksSinceLastTimeCode);

  // Record the time code:
  usingSource()->setTimeCode(time_code_hours, time_code_minutes,
                             time_code_seconds, 0, 0);
    // Note: Because the GOV header can appear anywhere (not just at a 1s point), we
    // don't pass "fTotalTicksSinceLastTimeCode" as the "picturesSinceLastGOP" parameter.
  fSecondsSinceLastTimeCode = 0;
  if (fixed_vop_rate) fTotalTicksSinceLastTimeCode = 0;

  setParseState(PARSING_VIDEO_OBJECT_PLANE);

  return curFrameSize();
}

unsigned MPEG4VideoStreamParser::parseVideoObjectPlane() {
#ifdef DEBUG
  fprintf(stderr, "#parsing VideoObjectPlane\n");
#endif
  // Note that we've already read the VOP_START_CODE
  save4Bytes(VOP_START_CODE);

  // Get the "vop_coding_type" from the next byte:
  u_int8_t nextByte = get1Byte(); saveByte(nextByte);
  u_int8_t vop_coding_type = nextByte>>6;

  // Next, get the "modulo_time_base" by counting the '1' bits that follow.
  // We look at the next 32-bits only.  This should be enough in most cases.
  u_int32_t next4Bytes = get4Bytes();
  u_int32_t timeInfo = (nextByte<<(32-6))|(next4Bytes>>6);
  unsigned modulo_time_base = 0;
  u_int32_t mask = 0x80000000;
  while ((timeInfo&mask) != 0) {
    ++modulo_time_base;
    mask >>= 1;
  }
  mask >>= 1;

  // Check the following marker bit:
  if ((timeInfo&mask) == 0) {
    usingSource()->envir() << "MPEG4VideoStreamParser::parseVideoObjectPlane(): marker bit not set!\n";
  }
  mask >>= 1;

  // Then, get the "vop_time_increment".
  // First, make sure we have enough bits left for this:
  if ((mask>>(fNumVTIRBits-1)) == 0) {
    usingSource()->envir() << "MPEG4VideoStreamParser::parseVideoObjectPlane(): 32-bits are not enough to get \"vop_time_increment\"!\n";
  }
  unsigned vop_time_increment = 0;
  for (unsigned i = 0; i < fNumVTIRBits; ++i) {
    vop_time_increment |= timeInfo&mask;
    mask >>= 1;
  }
  while (mask != 0) {
    vop_time_increment >>= 1;
    mask >>= 1;
  }
#ifdef DEBUG
  fprintf(stderr, "vop_coding_type: %d(%c), modulo_time_base: %d, vop_time_increment: %d\n", vop_coding_type, "IPBS"[vop_coding_type], modulo_time_base, vop_time_increment);
#endif

  // Now, copy all bytes that we see, up until we reach a code of some sort:
  saveToNextCode(next4Bytes);

  // Update our counters based on the frame timing information that we saw:
  if (fixed_vop_time_increment > 0) {
    // This is a 'fixed_vop_rate' stream.  Use 'fixed_vop_time_increment':
    usingSource()->fPictureCount += fixed_vop_time_increment;
    if (vop_time_increment > 0 || modulo_time_base > 0) {
      fTotalTicksSinceLastTimeCode += fixed_vop_time_increment;
      // Note: "fSecondsSinceLastTimeCode" and "fPrevNewTotalTicks" are not used.
    }
  } else {
    // Use 'vop_time_increment':
    unsigned newTotalTicks
      = (fSecondsSinceLastTimeCode + modulo_time_base)*vop_time_increment_resolution
      + vop_time_increment;
    if (newTotalTicks == fPrevNewTotalTicks && fPrevNewTotalTicks > 0) {
      // This is apparently a buggy MPEG-4 video stream, because
      // "vop_time_increment" did not change.  Overcome this error,
      // by pretending that it did change.
#ifdef DEBUG
      fprintf(stderr, "Buggy MPEG-4 video stream: \"vop_time_increment\" did not change!\n");
#endif
      // The following assumes that we don't have 'B' frames.  If we do, then TARFU!
      usingSource()->fPictureCount += vop_time_increment;
      fTotalTicksSinceLastTimeCode += vop_time_increment;
      fSecondsSinceLastTimeCode += modulo_time_base;
    } else {
      if (newTotalTicks < fPrevNewTotalTicks && vop_coding_type != 2/*B*/
	  && modulo_time_base == 0 && vop_time_increment == 0 && !fJustSawTimeCode) {
	// This is another kind of buggy MPEG-4 video stream, in which
	// "vop_time_increment" wraps around, but without
	// "modulo_time_base" changing (or just having had a new time code).
	// Overcome this by pretending that "vop_time_increment" *did* wrap around:
#ifdef DEBUG
	fprintf(stderr, "Buggy MPEG-4 video stream: \"vop_time_increment\" wrapped around, but without \"modulo_time_base\" changing!\n");
#endif
	++fSecondsSinceLastTimeCode;
	newTotalTicks += vop_time_increment_resolution;
      }
      fPrevNewTotalTicks = newTotalTicks;
      if (vop_coding_type != 2/*B*/) {
	int pictureCountDelta = newTotalTicks - fTotalTicksSinceLastTimeCode;
	if (pictureCountDelta <= 0) pictureCountDelta = fPrevPictureCountDelta;
	    // ensures that the picture count is always increasing
	usingSource()->fPictureCount += pictureCountDelta;
	fPrevPictureCountDelta = pictureCountDelta;
	fTotalTicksSinceLastTimeCode = newTotalTicks;
	fSecondsSinceLastTimeCode += modulo_time_base;
      }
    }
  }
  fJustSawTimeCode = False; // for next time

  // The next thing to parse depends on the code that we just saw,
  // but we are assumed to have ended the current picture:
  usingSource()->fPictureEndMarker = True; // HACK #####
  switch (next4Bytes) {
  case VISUAL_OBJECT_SEQUENCE_END_CODE: {
    setParseState(PARSING_VISUAL_OBJECT_SEQUENCE_END_CODE);
    break;
  }
  case VISUAL_OBJECT_SEQUENCE_START_CODE: {
    setParseState(PARSING_VISUAL_OBJECT_SEQUENCE_SEEN_CODE);
    break;
  }
  case VISUAL_OBJECT_START_CODE: {
    setParseState(PARSING_VISUAL_OBJECT);
    break;
  }
  case GROUP_VOP_START_CODE: {
    setParseState(PARSING_GROUP_OF_VIDEO_OBJECT_PLANE);
    break;
  }
  case VOP_START_CODE: {
    setParseState(PARSING_VIDEO_OBJECT_PLANE);
    break;
  }
  default: {
    if (isVideoObjectStartCode(next4Bytes)) {
      setParseState(PARSING_VIDEO_OBJECT_LAYER);
    } else if (isVideoObjectLayerStartCode(next4Bytes)){
      // copy all bytes that we see, up until we reach a VOP_START_CODE:
      u_int32_t next4Bytes = get4Bytes();
      while (next4Bytes != VOP_START_CODE) {
	saveToNextCode(next4Bytes);
      }
      setParseState(PARSING_VIDEO_OBJECT_PLANE);
    } else {
      usingSource()->envir() << "MPEG4VideoStreamParser::parseVideoObjectPlane(): Saw unexpected code "
			     << (void*)next4Bytes << "\n";
      setParseState(PARSING_VIDEO_OBJECT_PLANE); // the safest way to recover...
    }
    break;
  }
  }

  // Compute this frame's presentation time:
  usingSource()->computePresentationTime(fTotalTicksSinceLastTimeCode);

  return curFrameSize();
}

unsigned MPEG4VideoStreamParser::parseVisualObjectSequenceEndCode() {
#ifdef DEBUG
  fprintf(stderr, "parsing VISUAL_OBJECT_SEQUENCE_END_CODE\n");
#endif
  // Note that we've already read the VISUAL_OBJECT_SEQUENCE_END_CODE
  save4Bytes(VISUAL_OBJECT_SEQUENCE_END_CODE);

  setParseState(PARSING_VISUAL_OBJECT_SEQUENCE);

  // Treat this as if we had ended a picture:
  usingSource()->fPictureEndMarker = True; // HACK #####

  return curFrameSize();
}
