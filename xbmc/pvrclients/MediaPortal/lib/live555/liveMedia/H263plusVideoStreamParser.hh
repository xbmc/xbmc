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
// A filter that breaks up an H263 video stream into frames.
// derived from MPEG4IP h263.c
// Author Benhard Feiten

#ifndef _H263PLUS_VIDEO_STREAM_PARSER_HH
#define _H263PLUS_VIDEO_STREAM_PARSER_HH

#ifndef _STREAM_PARSER_HH
#include "StreamParser.hh"
#endif


// Default timescale for H.263 (1000ms)
#define H263_TIMESCALE 1000

// Default H263 frame rate (30fps)
#define H263_BASIC_FRAME_RATE 30

// Minimum number of bytes needed to parse an H263 header
#define H263_REQUIRE_HEADER_SIZE_BYTES 5

// Number of bytes the start code requries
#define H263_STARTCODE_SIZE_BYTES 3

// This is the input buffer's size. It should contain
// 1 frame with the following start code
#define H263_BUFFER_SIZE 256 * 1024

//  additionalBytesNeeded - indicates how many additional bytes are to be read
//  from the next frame's header (over the 3 bytes that are already read).
#define ADDITIONAL_BYTES_NEEDED H263_REQUIRE_HEADER_SIZE_BYTES - H263_STARTCODE_SIZE_BYTES

// The default max different (in %) betwqeen max and average bitrates
#define H263_DEFAULT_CBR_TOLERANCE  10



// The following structure holds information extracted from each frame's header:
typedef struct _H263INFO {
  u_int8_t  tr;                 // Temporal Reference, used in duration calculation
  u_int16_t width;              // Width of the picture
  u_int16_t height;             // Height of the picture
  bool      isSyncFrame;        // Frame type (true = I frame = "sync" frame)
} H263INFO;

typedef struct _MaxBitrate_CTX {
  u_int32_t  bitrateTable[H263_BASIC_FRAME_RATE];// Window of 1 second
  u_int32_t  windowBitrate;              // The bitrate of the current window
  u_int32_t  maxBitrate;                 // The up-to-date maximum bitrate
  u_int32_t  tableIndex;                 // The next TR unit to update
} MaxBitrate_CTX;


class H263plusVideoStreamParser : public StreamParser {

public:
   H263plusVideoStreamParser( class H263plusVideoStreamFramer* usingSource,
			                  FramedSource* inputSource);

   virtual ~H263plusVideoStreamParser();

   void registerReadInterest(unsigned char* to, unsigned maxSize);

   unsigned parse(u_int64_t & currentDuration);    // returns the size of the frame  that was acquired, or 0 if none
   unsigned numTruncatedBytes() const { return fNumTruncatedBytes; }  // The number of truncated bytes (if any)


protected:
//  H263plusVideoStreamFramer* usingSource() {
//    return (H263plusVideoStreamFramer*)fUsingSource;
//  }
  void setParseState();

//   void setParseState(H263plusParseState parseState);


private:
   int       parseH263Frame( );
   bool      ParseShortHeader(u_int8_t *headerBuffer, H263INFO *outputInfoStruct);
   void      GetMaxBitrate( MaxBitrate_CTX *ctx, u_int32_t frameSize, u_int8_t frameTRDiff);
   u_int64_t CalculateDuration(u_int8_t trDiff);
   bool      GetWidthAndHeight( u_int8_t  fmt, u_int16_t *width, u_int16_t *height);
   u_int8_t  GetTRDifference( u_int8_t nextTR, u_int8_t currentTR);

  virtual void restoreSavedParserState();

protected:
   class H263plusVideoStreamFramer* fUsingSource;

   unsigned char* fTo;
   unsigned       fMaxSize;
   unsigned char* fStartOfFrame;
   unsigned char* fSavedTo;
   unsigned char* fLimit;
   unsigned       fNumTruncatedBytes;
   unsigned       fSavedNumTruncatedBytes;

private:
   H263INFO       fNextInfo;       // Holds information about the next frame
   H263INFO       fCurrentInfo;    // Holds information about the current frame
   MaxBitrate_CTX fMaxBitrateCtx;  // Context for the GetMaxBitrate function
   char           fStates[3][256];
   u_int8_t       fNextHeader[H263_REQUIRE_HEADER_SIZE_BYTES];

  u_int32_t fnextTR;   // The next frame's presentation time in TR units
  u_int64_t fcurrentPT;   // The current frame's presentation time in milli-seconds

};

#endif
