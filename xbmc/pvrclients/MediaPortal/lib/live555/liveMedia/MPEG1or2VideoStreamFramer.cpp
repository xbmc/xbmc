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
// A filter that breaks up an MPEG 1 or 2 video elementary stream into
//   frames for: Video_Sequence_Header, GOP_Header, Picture_Header
// Implementation

#include "MPEG1or2VideoStreamFramer.hh"
#include "MPEGVideoStreamParser.hh"
#include <string.h>

////////// MPEG1or2VideoStreamParser definition //////////

// An enum representing the current state of the parser:
enum MPEGParseState {
  PARSING_VIDEO_SEQUENCE_HEADER,
  PARSING_VIDEO_SEQUENCE_HEADER_SEEN_CODE,
  PARSING_GOP_HEADER,
  PARSING_GOP_HEADER_SEEN_CODE,
  PARSING_PICTURE_HEADER,
  PARSING_SLICE
};

#define VSH_MAX_SIZE 1000

class MPEG1or2VideoStreamParser: public MPEGVideoStreamParser {
public:
  MPEG1or2VideoStreamParser(MPEG1or2VideoStreamFramer* usingSource,
			FramedSource* inputSource,
			Boolean iFramesOnly, double vshPeriod);
  virtual ~MPEG1or2VideoStreamParser();

private: // redefined virtual functions:
  virtual void flushInput();
  virtual unsigned parse();

private:
  void reset();

  MPEG1or2VideoStreamFramer* usingSource() {
    return (MPEG1or2VideoStreamFramer*)fUsingSource;
  }
  void setParseState(MPEGParseState parseState);

  unsigned parseVideoSequenceHeader(Boolean haveSeenStartCode);
  unsigned parseGOPHeader(Boolean haveSeenStartCode);
  unsigned parsePictureHeader();
  unsigned parseSlice();

private:
  MPEGParseState fCurrentParseState;
  unsigned fPicturesSinceLastGOP;
      // can be used to compute timestamp for a video_sequence_header
  unsigned short fCurPicTemporalReference;
      // used to compute slice timestamp
  unsigned char fCurrentSliceNumber; // set when parsing a slice

  // A saved copy of the most recently seen 'video_sequence_header',
  // in case we need to insert it into the stream periodically:
  unsigned char fSavedVSHBuffer[VSH_MAX_SIZE];
  unsigned fSavedVSHSize;
  double fSavedVSHTimestamp;
  double fVSHPeriod;
  Boolean fIFramesOnly, fSkippingCurrentPicture;

  void saveCurrentVSH();
  Boolean needToUseSavedVSH();
  unsigned useSavedVSH(); // returns the size of the saved VSH
};


////////// MPEG1or2VideoStreamFramer implementation //////////

MPEG1or2VideoStreamFramer::MPEG1or2VideoStreamFramer(UsageEnvironment& env,
						     FramedSource* inputSource,
						     Boolean iFramesOnly,
						     double vshPeriod,
						     Boolean createParser)
  : MPEGVideoStreamFramer(env, inputSource) {
  fParser = createParser
    ? new MPEG1or2VideoStreamParser(this, inputSource,
				    iFramesOnly, vshPeriod)
    : NULL;
}

MPEG1or2VideoStreamFramer::~MPEG1or2VideoStreamFramer() {
}

MPEG1or2VideoStreamFramer*
MPEG1or2VideoStreamFramer::createNew(UsageEnvironment& env,
				 FramedSource* inputSource,
				 Boolean iFramesOnly,
				 double vshPeriod) {
  // Need to add source type checking here???  #####
  return new MPEG1or2VideoStreamFramer(env, inputSource, iFramesOnly, vshPeriod);
}

double MPEG1or2VideoStreamFramer::getCurrentPTS() const {
  return fPresentationTime.tv_sec + fPresentationTime.tv_usec/1000000.0;
}

Boolean MPEG1or2VideoStreamFramer::isMPEG1or2VideoStreamFramer() const {
  return True;
}

////////// MPEG1or2VideoStreamParser implementation //////////

MPEG1or2VideoStreamParser
::MPEG1or2VideoStreamParser(MPEG1or2VideoStreamFramer* usingSource,
			FramedSource* inputSource,
			Boolean iFramesOnly, double vshPeriod)
  : MPEGVideoStreamParser(usingSource, inputSource),
    fCurrentParseState(PARSING_VIDEO_SEQUENCE_HEADER),
    fVSHPeriod(vshPeriod), fIFramesOnly(iFramesOnly) {
  reset();
}

MPEG1or2VideoStreamParser::~MPEG1or2VideoStreamParser() {
}

void MPEG1or2VideoStreamParser::setParseState(MPEGParseState parseState) {
  fCurrentParseState = parseState;
  MPEGVideoStreamParser::setParseState();
}

void MPEG1or2VideoStreamParser::reset() {
  fPicturesSinceLastGOP = 0;
  fCurPicTemporalReference = 0;
  fCurrentSliceNumber = 0;
  fSavedVSHSize = 0;
  fSkippingCurrentPicture = False;
}

void MPEG1or2VideoStreamParser::flushInput() {
  reset();
  StreamParser::flushInput();
  if (fCurrentParseState != PARSING_VIDEO_SEQUENCE_HEADER) {
    setParseState(PARSING_GOP_HEADER); // start from the next GOP
  }
}

unsigned MPEG1or2VideoStreamParser::parse() {
  try {
    switch (fCurrentParseState) {
    case PARSING_VIDEO_SEQUENCE_HEADER: {
      return parseVideoSequenceHeader(False);
    }
    case PARSING_VIDEO_SEQUENCE_HEADER_SEEN_CODE: {
      return parseVideoSequenceHeader(True);
    }
    case PARSING_GOP_HEADER: {
      return parseGOPHeader(False);
    }
    case PARSING_GOP_HEADER_SEEN_CODE: {
      return parseGOPHeader(True);
    }
    case PARSING_PICTURE_HEADER: {
      return parsePictureHeader();
    }
    case PARSING_SLICE: {
      return parseSlice();
    }
    default: {
      return 0; // shouldn't happen
    }
    }
  } catch (int /*e*/) {
#ifdef DEBUG
    fprintf(stderr, "MPEG1or2VideoStreamParser::parse() EXCEPTION (This is normal behavior - *not* an error)\n");
#endif
    return 0;  // the parsing got interrupted
  }
}

void MPEG1or2VideoStreamParser::saveCurrentVSH() {
  unsigned frameSize = curFrameSize();
  if (frameSize > sizeof fSavedVSHBuffer) return; // too big to save

  memmove(fSavedVSHBuffer, fStartOfFrame, frameSize);
  fSavedVSHSize = frameSize;
  fSavedVSHTimestamp = usingSource()->getCurrentPTS();
}

Boolean MPEG1or2VideoStreamParser::needToUseSavedVSH() {
  return usingSource()->getCurrentPTS() > fSavedVSHTimestamp+fVSHPeriod
    && fSavedVSHSize > 0;
}

unsigned MPEG1or2VideoStreamParser::useSavedVSH() {
  unsigned bytesToUse = fSavedVSHSize;
  unsigned maxBytesToUse = fLimit - fStartOfFrame;
  if (bytesToUse > maxBytesToUse) bytesToUse = maxBytesToUse;

  memmove(fStartOfFrame, fSavedVSHBuffer, bytesToUse);

  // Also reset the saved timestamp:
  fSavedVSHTimestamp = usingSource()->getCurrentPTS();

#ifdef DEBUG
  fprintf(stderr, "used saved video_sequence_header (%d bytes)\n", bytesToUse);
#endif
  return bytesToUse;
}

#define VIDEO_SEQUENCE_HEADER_START_CODE 0x000001B3
#define GROUP_START_CODE                 0x000001B8
#define PICTURE_START_CODE               0x00000100
#define SEQUENCE_END_CODE                0x000001B7

static double const frameRateFromCode[] = {
  0.0,          // forbidden
  24000/1001.0, // approx 23.976
  24.0,
  25.0,
  30000/1001.0, // approx 29.97
  30.0,
  50.0,
  60000/1001.0, // approx 59.94
  60.0,
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0           // reserved
};

unsigned MPEG1or2VideoStreamParser
::parseVideoSequenceHeader(Boolean haveSeenStartCode) {
#ifdef DEBUG
  fprintf(stderr, "parsing video sequence header\n");
#endif
  unsigned first4Bytes;
  if (!haveSeenStartCode) {
    while ((first4Bytes = test4Bytes()) != VIDEO_SEQUENCE_HEADER_START_CODE) {
#ifdef DEBUG
      fprintf(stderr, "ignoring non video sequence header: 0x%08x\n", first4Bytes);
#endif
      get1Byte(); setParseState(PARSING_VIDEO_SEQUENCE_HEADER);
          // ensures we progress over bad data
    }
    first4Bytes = get4Bytes();
  } else {
    // We've already seen the start code
    first4Bytes = VIDEO_SEQUENCE_HEADER_START_CODE;
  }
  save4Bytes(first4Bytes);

  // Next, extract the size and rate parameters from the next 8 bytes
  unsigned paramWord1 = get4Bytes();
  save4Bytes(paramWord1);
  unsigned next4Bytes = get4Bytes();
#ifdef DEBUG
  unsigned short horizontal_size_value   = (paramWord1&0xFFF00000)>>(32-12);
  unsigned short vertical_size_value     = (paramWord1&0x000FFF00)>>8;
  unsigned char aspect_ratio_information = (paramWord1&0x000000F0)>>4;
#endif
  unsigned char frame_rate_code          = (paramWord1&0x0000000F);
  usingSource()->fFrameRate = frameRateFromCode[frame_rate_code];
#ifdef DEBUG
  unsigned bit_rate_value                = (next4Bytes&0xFFFFC000)>>(32-18);
  unsigned vbv_buffer_size_value         = (next4Bytes&0x00001FF8)>>3;
  fprintf(stderr, "horizontal_size_value: %d, vertical_size_value: %d, aspect_ratio_information: %d, frame_rate_code: %d (=>%f fps), bit_rate_value: %d (=>%d bps), vbv_buffer_size_value: %d\n", horizontal_size_value, vertical_size_value, aspect_ratio_information, frame_rate_code, usingSource()->fFrameRate, bit_rate_value, bit_rate_value*400, vbv_buffer_size_value);
#endif

  // Now, copy all bytes that we see, up until we reach a GROUP_START_CODE
  // or a PICTURE_START_CODE:
  do {
    saveToNextCode(next4Bytes);
  } while (next4Bytes != GROUP_START_CODE && next4Bytes != PICTURE_START_CODE);

  setParseState((next4Bytes == GROUP_START_CODE)
		? PARSING_GOP_HEADER_SEEN_CODE : PARSING_PICTURE_HEADER);

  // Compute this frame's timestamp by noting how many pictures we've seen
  // since the last GOP header:
  usingSource()->computePresentationTime(fPicturesSinceLastGOP);

  // Save this video_sequence_header, in case we need to insert a copy
  // into the stream later:
  saveCurrentVSH();

  return curFrameSize();
}

unsigned MPEG1or2VideoStreamParser::parseGOPHeader(Boolean haveSeenStartCode) {
  // First check whether we should insert a previously-saved
  // 'video_sequence_header' here:
  if (needToUseSavedVSH()) return useSavedVSH();

#ifdef DEBUG
  fprintf(stderr, "parsing GOP header\n");
#endif
  unsigned first4Bytes;
  if (!haveSeenStartCode) {
    while ((first4Bytes = test4Bytes()) != GROUP_START_CODE) {
#ifdef DEBUG
      fprintf(stderr, "ignoring non GOP start code: 0x%08x\n", first4Bytes);
#endif
      get1Byte(); setParseState(PARSING_GOP_HEADER);
          // ensures we progress over bad data
    }
    first4Bytes = get4Bytes();
  } else {
    // We've already seen the GROUP_START_CODE
    first4Bytes = GROUP_START_CODE;
  }
  save4Bytes(first4Bytes);

  // Next, extract the (25-bit) time code from the next 4 bytes:
  unsigned next4Bytes = get4Bytes();
  unsigned time_code = (next4Bytes&0xFFFFFF80)>>(32-25);
#if defined(DEBUG) || defined(DEBUG_TIMESTAMPS)
  Boolean drop_frame_flag     = (time_code&0x01000000) != 0;
#endif
  unsigned time_code_hours    = (time_code&0x00F80000)>>19;
  unsigned time_code_minutes  = (time_code&0x0007E000)>>13;
  unsigned time_code_seconds  = (time_code&0x00000FC0)>>6;
  unsigned time_code_pictures = (time_code&0x0000003F);
#if defined(DEBUG) || defined(DEBUG_TIMESTAMPS)
  fprintf(stderr, "time_code: 0x%07x, drop_frame %d, hours %d, minutes %d, seconds %d, pictures %d\n", time_code, drop_frame_flag, time_code_hours, time_code_minutes, time_code_seconds, time_code_pictures);
#endif
#ifdef DEBUG
  Boolean closed_gop  = (next4Bytes&0x00000040) != 0;
  Boolean broken_link = (next4Bytes&0x00000020) != 0;
  fprintf(stderr, "closed_gop: %d, broken_link: %d\n", closed_gop, broken_link);
#endif

  // Now, copy all bytes that we see, up until we reach a PICTURE_START_CODE:
  do {
    saveToNextCode(next4Bytes);
  } while (next4Bytes != PICTURE_START_CODE);

  // Record the time code:
  usingSource()->setTimeCode(time_code_hours, time_code_minutes,
			     time_code_seconds, time_code_pictures,
			     fPicturesSinceLastGOP);

  fPicturesSinceLastGOP = 0;

  // Compute this frame's timestamp:
  usingSource()->computePresentationTime(0);

  setParseState(PARSING_PICTURE_HEADER);

  return curFrameSize();
}

inline Boolean isSliceStartCode(unsigned fourBytes) {
  if ((fourBytes&0xFFFFFF00) != 0x00000100) return False;

  unsigned char lastByte = fourBytes&0xFF;
  return lastByte <= 0xAF && lastByte >= 1;
}

unsigned MPEG1or2VideoStreamParser::parsePictureHeader() {
#ifdef DEBUG
  fprintf(stderr, "parsing picture header\n");
#endif
  // Note that we've already read the PICTURE_START_CODE
  // Next, extract the temporal reference from the next 4 bytes:
  unsigned next4Bytes = get4Bytes();
  unsigned short temporal_reference = (next4Bytes&0xFFC00000)>>(32-10);
  unsigned char picture_coding_type = (next4Bytes&0x00380000)>>19;
#ifdef DEBUG
  unsigned short vbv_delay          = (next4Bytes&0x0007FFF8)>>3;
  fprintf(stderr, "temporal_reference: %d, picture_coding_type: %d, vbv_delay: %d\n", temporal_reference, picture_coding_type, vbv_delay);
#endif

  fSkippingCurrentPicture = fIFramesOnly && picture_coding_type != 1;
  if (fSkippingCurrentPicture) {
    // Skip all bytes that we see, up until we reach a slice_start_code:
    do {
      skipToNextCode(next4Bytes);
    } while (!isSliceStartCode(next4Bytes));
  } else {
    // Save the PICTURE_START_CODE that we've already read:
    save4Bytes(PICTURE_START_CODE);

    // Copy all bytes that we see, up until we reach a slice_start_code:
    do {
      saveToNextCode(next4Bytes);
    } while (!isSliceStartCode(next4Bytes));
  }

  setParseState(PARSING_SLICE);

  fCurrentSliceNumber = next4Bytes&0xFF;

  // Record the temporal reference:
  fCurPicTemporalReference = temporal_reference;

  // Compute this frame's timestamp:
  usingSource()->computePresentationTime(fCurPicTemporalReference);

  if (fSkippingCurrentPicture) {
    return parse(); // try again, until we get a non-skipped frame
  } else {
    return curFrameSize();
  }
}

unsigned MPEG1or2VideoStreamParser::parseSlice() {
  // Note that we've already read the slice_start_code:
  unsigned next4Bytes = PICTURE_START_CODE|fCurrentSliceNumber;
#ifdef DEBUG_SLICE
  fprintf(stderr, "parsing slice: 0x%08x\n", next4Bytes);
#endif

  if (fSkippingCurrentPicture) {
    // Skip all bytes that we see, up until we reach a code of some sort:
    skipToNextCode(next4Bytes);
  } else {
    // Copy all bytes that we see, up until we reach a code of some sort:
    saveToNextCode(next4Bytes);
  }

  // The next thing to parse depends on the code that we just saw:
  if (isSliceStartCode(next4Bytes)) { // common case
    setParseState(PARSING_SLICE);
    fCurrentSliceNumber = next4Bytes&0xFF;
  } else {
    // Because we don't see any more slices, we are assumed to have ended
    // the current picture:
    ++fPicturesSinceLastGOP;
    ++usingSource()->fPictureCount;
    usingSource()->fPictureEndMarker = True; // HACK #####

    switch (next4Bytes) {
    case SEQUENCE_END_CODE: {
      setParseState(PARSING_VIDEO_SEQUENCE_HEADER);
      break;
    }
    case VIDEO_SEQUENCE_HEADER_START_CODE: {
      setParseState(PARSING_VIDEO_SEQUENCE_HEADER_SEEN_CODE);
      break;
    }
    case GROUP_START_CODE: {
      setParseState(PARSING_GOP_HEADER_SEEN_CODE);
      break;
    }
    case PICTURE_START_CODE: {
      setParseState(PARSING_PICTURE_HEADER);
      break;
    }
    default: {
      usingSource()->envir() << "MPEG1or2VideoStreamParser::parseSlice(): Saw unexpected code "
			    << (void*)next4Bytes << "\n";
      setParseState(PARSING_SLICE); // the safest way to recover...
      break;
    }
    }
  }

  // Compute this frame's timestamp:
  usingSource()->computePresentationTime(fCurPicTemporalReference);

  if (fSkippingCurrentPicture) {
    return parse(); // try again, until we get a non-skipped frame
  } else {
    return curFrameSize();
  }
}
