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
// Author Bernhard Feiten
// A filter that breaks up an H.263plus video stream into frames.
// Based on MPEG4IP/mp4creator/h263.c

#include "H263plusVideoStreamParser.hh"
#include "H263plusVideoStreamFramer.hh"
//#include <string.h>
//#include "GroupsockHelper.hh"


H263plusVideoStreamParser::H263plusVideoStreamParser(
                              H263plusVideoStreamFramer* usingSource,
                              FramedSource* inputSource)
                              : StreamParser(inputSource,
                                   FramedSource::handleClosure,
                                   usingSource,
                                   &H263plusVideoStreamFramer::continueReadProcessing,
                                   usingSource),
                                fUsingSource(usingSource),
                                fnextTR(0),
                                fcurrentPT(0)
{
   memset(fStates, 0, sizeof(fStates));
   memset(&fNextInfo, 0, sizeof(fNextInfo));
   memset(&fCurrentInfo, 0, sizeof(fCurrentInfo));
   memset(&fMaxBitrateCtx, 0, sizeof(fMaxBitrateCtx));
   memset(fNextHeader,0, H263_REQUIRE_HEADER_SIZE_BYTES);
}

///////////////////////////////////////////////////////////////////////////////
H263plusVideoStreamParser::~H263plusVideoStreamParser()
{
}

///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamParser::restoreSavedParserState()
{
   StreamParser::restoreSavedParserState();
   fTo = fSavedTo;
   fNumTruncatedBytes = fSavedNumTruncatedBytes;
}

///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamParser::setParseState()
{
   fSavedTo = fTo;
   fSavedNumTruncatedBytes = fNumTruncatedBytes;
   saveParserState();  // Needed for the parsing process in StreamParser
}


///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamParser::registerReadInterest(
                                   unsigned char* to,
                                   unsigned maxSize)
{
   fStartOfFrame = fTo = fSavedTo = to;
   fLimit = to + maxSize;
   fMaxSize = maxSize;
   fNumTruncatedBytes = fSavedNumTruncatedBytes = 0;
}

///////////////////////////////////////////////////////////////////////////////
// parse() ,  derived from H263Creator of MPEG4IP, h263.c
unsigned H263plusVideoStreamParser::parse(u_int64_t & currentDuration)
{

//   u_int8_t       frameBuffer[H263_BUFFER_SIZE]; // The input buffer
                 // Pointer which tells LoadNextH263Object where to read data to
//   u_int8_t*      pFrameBuffer = fTo + H263_REQUIRE_HEADER_SIZE_BYTES;
   u_int32_t      frameSize;        // The current frame size
                                // Pointer to receive address of the header data
//   u_int8_t*      pCurrentHeader;// = pFrameBuffer;
//   u_int64_t      currentDuration;  // The current frame's duration
   u_int8_t       trDifference;     // The current TR difference
                                   // The previous TR difference
//   u_int8_t       prevTrDifference = H263_BASIC_FRAME_RATE;
//   u_int64_t      totalDuration = 0;// Duration accumulator
//   u_int64_t      avgBitrate;       // Average bitrate
//   u_int64_t      totalBytes = 0;   // Size accumulator


   try    // The get data routines of the class FramedFilter returns an error when
   {      // the buffer is empty. This occurs at the beginning and at the end of the file.
      fCurrentInfo = fNextInfo;

      // Parse 1 frame
      // For the first time, only the first frame's header is returned.
      // The second time the full first frame is returned
      frameSize = parseH263Frame();

      currentDuration = 0;
      if ((frameSize > 0)){
         // We were able to acquire a frame from the input.

         // Parse the returned frame header (if any)
         if (!ParseShortHeader(fTo, &fNextInfo)) {
#ifdef DEBUG
	   fprintf(stderr,"H263plusVideoStreamParser: Fatal error\n");
#endif
	 }

         trDifference = GetTRDifference(fNextInfo.tr, fCurrentInfo.tr);

         // calculate the current frame duration
         currentDuration = CalculateDuration(trDifference);

         // Accumulate the frame's size and duration for avgBitrate calculation
         //totalDuration += currentDuration;
         //totalBytes += frameSize;
         //  If needed, recalculate bitrate information
         //    if (h263Bitrates)
         //GetMaxBitrate(&fMaxBitrateCtx, frameSize, prevTrDifference);
         //prevTrDifference = trDifference;

	 setParseState(); // Needed for the parsing process in StreamParser
      }
   } catch (int /*e*/) {
#ifdef DEBUG
      fprintf(stderr, "H263plusVideoStreamParser::parse() EXCEPTION (This is normal behavior - *not* an error)\n");
#endif
      frameSize=0;
   }

   return frameSize;
}


///////////////////////////////////////////////////////////////////////////////
// parseH263Frame derived from LoadNextH263Object of MPEG4IP
// - service routine that reads a single frame from the input file.
// It shall fill the input buffer with data up until - and including - the
// next start code and shall report back both the number of bytes read and a
// pointer to the next start code. The first call to this function shall only
// yield a pointer with 0 data bytes and the last call to this function shall
// only yield data bytes with a NULL pointer as the next header.
//
// TODO: This function only supports valid bit streams. Upon error, it fails
// without the possibility to recover. A Better idea would be to skip frames
// until a parsable frame is read from the file.
//
// Parameters:
//      ppNextHeader - output parameter that upon return points to the location
//                     of the next frame's head in the buffer.
//                     This pointer shall be NULL for the last frame read.
// Returns the total number of bytes read.
// Uses FrameFileSource intantiated by constructor.
///////////////////////////////////////////////////////////////////////////////
int H263plusVideoStreamParser::parseH263Frame( )
{
   char     row = 0;
   u_int8_t * bufferIndex = fTo;
   // The buffer end which will allow the loop to leave place for
   // the additionalBytesNeeded
   u_int8_t * bufferEnd = fTo + fMaxSize - ADDITIONAL_BYTES_NEEDED - 1;

   memcpy(fTo, fNextHeader, H263_REQUIRE_HEADER_SIZE_BYTES);
   bufferIndex += H263_REQUIRE_HEADER_SIZE_BYTES;


   // The state table and the following loop implements a state machine enabling
   // us to read bytes from the file until (and inclusing) the requested
   // start code (00 00 8X) is found

   // Initialize the states array, if it hasn't been initialized yet...
   if (!fStates[0][0]) {
      // One 00 was read
      fStates[0][0] = 1;
      // Two sequential 0x00 ware read
      fStates[1][0] = fStates[2][0] = 2;
      // A full start code was read
      fStates[2][128] = fStates[2][129] = fStates[2][130] = fStates[2][131] = -1;
   }

   // Read data from file into the output buffer until either a start code
   // is found, or the end of file has been reached.
   do {
      *bufferIndex = get1Byte();
   } while ((bufferIndex < bufferEnd) &&                    // We have place in the buffer
            ((row = fStates[(unsigned char)row][*(bufferIndex++)]) != -1)); // Start code was not found

   if (row != -1) {
      fprintf(stderr, "%s: Buffer too small (%u)\n",
         "h263reader:", bufferEnd - fTo + ADDITIONAL_BYTES_NEEDED);
      return 0;
   }

   // Cool ... now we have a start code
   // Now we just have to read the additionalBytesNeeded
   getBytes(bufferIndex, ADDITIONAL_BYTES_NEEDED);
   memcpy(fNextHeader, bufferIndex - H263_STARTCODE_SIZE_BYTES, H263_REQUIRE_HEADER_SIZE_BYTES);

	int sz = bufferIndex - fTo - H263_STARTCODE_SIZE_BYTES;

   if (sz == 5) // first frame
      memcpy(fTo, fTo+H263_REQUIRE_HEADER_SIZE_BYTES, H263_REQUIRE_HEADER_SIZE_BYTES);

   return sz;
}


////////////////////////////////////////////////////////////////////////////////
// ParseShortHeader - service routine that accepts a buffer containing a frame
// header and extracts relevant codec information from it.
//
// NOTE: the first bit in the following commnets is 0 (zero).
//
//       0                   1                   2                   3
//       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |      PSC (Picture Start Code=22 bits)     |  (TR=8 bits)  |   >
//      |0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0|               |1 0>
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      <   (PTYPE=13 bits)   |
//      <. . .|(FMT)|Z|. . . .|
//      +-+-+-+-+-+-+-+-+-+-+-+
//      -> PTYPE.FMT contains a width/height identification
//      -> PTYPE.Z   is 1 for P-Frames, 0 for I-Frames
//      Note: When FMT is 111, there is an extended PTYPE...
//
// Inputs:
//      headerBuffer - pointer to the current header buffer
//      outputInfoStruct - pointer to the structure receiving the data
// Outputs:
//      This function returns a structure of important codec-specific
//      information (The Temporal Reference bits, width & height of the current
//      frame and the sync - or "frame type" - bit. It reports success or
//      failure to the calling function.
////////////////////////////////////////////////////////////////////////////////
bool H263plusVideoStreamParser::ParseShortHeader(
                                   u_int8_t *headerBuffer,
                                   H263INFO *outputInfoStruct)
{
   u_int8_t fmt = 0;
   // Extract temporal reference (TR) from the buffer (bits 22-29 inclusive)
   outputInfoStruct->tr  = (headerBuffer[2] << 6) & 0xC0; // 2 LS bits out of the 3rd byte
   outputInfoStruct->tr |= (headerBuffer[3] >> 2) & 0x3F; // 6 MS bits out of the 4th byte
   // Extract the FMT part of PTYPE from the buffer (bits 35-37 inclusive)
   fmt = (headerBuffer[4] >> 2) & 0x07; // bits 3-5 ouf of the 5th byte
   // If PTYPE is not supported, return a failure notice to the calling function
   // FIXME: PLUSPTYPE is not supported
   if (fmt == 0x07) {
      return false;
   }
   // If PTYPE is supported, calculate the current width and height according to
   // a predefined table
   if (!GetWidthAndHeight(fmt, &(outputInfoStruct->width),
                               &(outputInfoStruct->height))) {
      return false;
   }
   // Extract the frame-type bit, which is the 9th bit of PTYPE (bit 38)
   outputInfoStruct->isSyncFrame = !(headerBuffer[4] & 0x02);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// GetMaxBitrate- service routine that accepts frame information and
// derives bitrate information from it. This function uses a sliding window
// technique to calculate the maximum bitrates in any window of 1 second
// inside the file.
// The sliding window is implemented with a table of bitrates for the last
// second (30 entries - one entry per TR unit).
//
// Inputs:
//      ctx - context for this function
//      frameSize - the size of the current frame in bytes
//      frameTRDiff - the "duration" of the frame in TR units
// Outputs:
//      This function returns the up-to-date maximum bitrate
////////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamParser::GetMaxBitrate( MaxBitrate_CTX *ctx,
                                               u_int32_t      frameSize,
                                               u_int8_t       frameTRDiff)
{
   if (frameTRDiff == 0)
      return;

   // Calculate the current frame's bitrate as bits per TR unit (round the result
   // upwards)
   u_int32_t frameBitrate = frameSize * 8 / frameTRDiff + 1;

   // for each TRdiff received,
   while (frameTRDiff--) {
      // Subtract the oldest bitrate entry from the current bitrate
      ctx->windowBitrate -= ctx->bitrateTable[ctx->tableIndex];
      // Update the oldest bitrate entry with the current frame's bitrate
      ctx->bitrateTable[ctx->tableIndex] = frameBitrate;
      // Add the current frame's bitrate to the current bitrate
      ctx->windowBitrate += frameBitrate;
      // Check if we have a new maximum bitrate
      if (ctx->windowBitrate > ctx->maxBitrate) {
         ctx->maxBitrate = ctx->windowBitrate;
	  }
      // Advance the table index
      // Wrapping around the bitrateTable size
      ctx->tableIndex = (ctx->tableIndex + 1) %
        ( sizeof(ctx->bitrateTable) / sizeof(ctx->bitrateTable[0]) );
   }
}

////////////////////////////////////////////////////////////////////////////////
// CalculateDuration - service routine that calculates the current frame's
// duration in milli-seconds using it's duration in TR units.
//  - In order not to accumulate the calculation error, we are using the TR
// duration to calculate the current and the next frame's presentation time in
// milli-seconds.
//
// Inputs: trDiff - The current frame's duration in TR units
// Return: The current frame's duration in milli-seconds
////////////////////////////////////////////////////////////////////////////////
u_int64_t H263plusVideoStreamParser::CalculateDuration(u_int8_t trDiff)
{
  //static u_int32_t nextTR    = 0;   // The next frame's presentation time in TR units
  //static u_int64_t currentPT = 0;   // The current frame's presentation time in milli-seconds
  u_int64_t        nextPT;          // The next frame's presentation time in milli-seconds
  u_int64_t        duration;        // The current frame's duration in milli-seconds

  fnextTR += trDiff;
  // Calculate the next frame's presentation time, in milli-seconds
  nextPT = (fnextTR * 1001) / H263_BASIC_FRAME_RATE;
  // The frame's duration is the difference between the next presentation
  // time and the current presentation time.
  duration = nextPT - fcurrentPT;
  // "Remember" the next presentation time for the next time this function is called
  fcurrentPT = nextPT;

  return duration;
}

////////////////////////////////////////////////////////////////////////////////
bool H263plusVideoStreamParser::GetWidthAndHeight( u_int8_t  fmt,
                                                   u_int16_t *width,
                                                   u_int16_t *height)
{
   // The 'fmt' corresponds to bits 5-7 of the PTYPE
   static struct {
      u_int16_t width;
      u_int16_t height;
   } dimensionsTable[8] = {
	   { 0,    0 },      // 000 - 0 - forbidden, generates an error
	   { 128,  96 },     // 001 - 1 - Sub QCIF
	   { 176,  144 },    // 010 - 2 - QCIF
	   { 352,  288 },    // 011 - 3 - CIF
	   { 704,  576 },    // 100 - 4 - 4CIF
	   { 1409, 1152 },   // 101 - 5 - 16CIF
	   { 0,    0 },      // 110 - 6 - reserved, generates an error
	   { 0,    0 }       // 111 - 7 - extended, not supported by profile 0
   };

   if (fmt > 7)
      return false;

   *width  = dimensionsTable[fmt].width;
   *height = dimensionsTable[fmt].height;

   if (*width  == 0)
     return false;

   return true;
}

////////////////////////////////////////////////////////////////////////////////
u_int8_t H263plusVideoStreamParser::GetTRDifference(
                                              u_int8_t nextTR,
                                              u_int8_t currentTR)
{
   if (currentTR > nextTR) {
      // Wrap around 255...
      return nextTR + (256 - currentTR);
   } else {
      return nextTR - currentTR;
   }
}







////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// this is the h263.c file of MPEG4IP mp4creator
/*
#include "mp4creator.h"

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
// The default max different (in %) betwqeen max and average bitrates
#define H263_DEFAULT_CBR_TOLERANCE  10

// The following structure holds information extracted from each frame's header:
typedef struct _H263INFO {
  u_int8_t  tr;                 // Temporal Reference, used in duration calculation
  u_int16_t width;              // Width of the picture
  u_int16_t height;             // Height of the picture
  bool      isSyncFrame;        // Frame type (true = I frame = "sync" frame)
} H263INFO;

// Context for the GetMaxBitrate function
typedef struct _MaxBitrate_CTX {
  u_int32_t  bitrateTable[H263_BASIC_FRAME_RATE];// Window of 1 second
  u_int32_t  windowBitrate;              // The bitrate of the current window
  u_int32_t  maxBitrate;                 // The up-to-date maximum bitrate
  u_int32_t  tableIndex;                 // The next TR unit to update
} MaxBitrate_CTX;

// Forward declarations:
static int LoadNextH263Object(  FILE           *inputFileHandle,
                                u_int8_t       *frameBuffer,
                                u_int32_t      *frameBufferSize,
                                u_int32_t       additionalBytesNeeded,
                                u_int8_t      **ppNextHeader);

static bool ParseShortHeader(   u_int8_t       *headerBuffer,
                                H263INFO       *outputInfoStruct);

static u_int8_t GetTRDifference(u_int8_t        nextTR,
                                u_int8_t        currentTR);

static void GetMaxBitrate(      MaxBitrate_CTX *ctx,
                                u_int32_t       frameSize,
                                u_int8_t        frameTRDiff);

static MP4Duration CalculateDuration(u_int8_t   trDiff);

static bool GetWidthAndHeight(  u_int8_t        fmt,
                                u_int16_t      *width,
                                u_int16_t      *height);

static char   states[3][256];
/ *
 * H263Creator - Main function
 * Inputs:
 *      outputFileHandle - The handle of the output file
 *      inputFileHandle - The handle of the input file
 *      Codec-specific parameters:
 *              H263Level - H.263 Level used for this track
 *              H263Profile - H.263 Profile used for this track
 *              H263Bitrates - A Parameter indicating whether the function
 *                             should calculate H263 bitrates or not.
 *              cbrTolerance - CBR tolerance indicates when to set the
 *                             average bitrate.
 * Outputs:
 *      This function returns either the track ID of the newly added track upon
 *      success or a predefined value representing an erroneous state.
 * /
MP4TrackId H263Creator(MP4FileHandle outputFileHandle,
                       FILE*         inputFileHandle,
                       u_int8_t      h263Profile,
                       u_int8_t      h263Level,
                       bool          h263Bitrates,
                       u_int8_t      cbrTolerance)
{
  H263INFO       nextInfo;   // Holds information about the next frame
  H263INFO       currentInfo;// Holds information about the current frame
  MaxBitrate_CTX maxBitrateCtx;// Context for the GetMaxBitrate function
  memset(&nextInfo, 0, sizeof(nextInfo));
  memset(&currentInfo, 0, sizeof(currentInfo));
  memset(&maxBitrateCtx, 0, sizeof(maxBitrateCtx));
  memset(states, 0, sizeof(states));
  u_int8_t       frameBuffer[H263_BUFFER_SIZE]; // The input buffer
                 // Pointer which tells LoadNextH263Object where to read data to
  u_int8_t*      pFrameBuffer = frameBuffer + H263_REQUIRE_HEADER_SIZE_BYTES;
  u_int32_t      frameSize;        // The current frame size
                                // Pointer to receive address of the header data
  u_int8_t*      pCurrentHeader = pFrameBuffer;
  MP4Duration    currentDuration;  // The current frame's duration
  u_int8_t       trDifference;     // The current TR difference
                                   // The previous TR difference
  u_int8_t       prevTrDifference = H263_BASIC_FRAME_RATE;
  MP4Duration    totalDuration = 0;// Duration accumulator
  MP4Duration    avgBitrate;       // Average bitrate
  u_int64_t      totalBytes = 0;   // Size accumulator
  MP4TrackId     trackId = MP4_INVALID_TRACK_ID; // Our MP4 track
  bool           stay = true;      // loop flag

  while (stay) {
    currentInfo = nextInfo;
    memmove(frameBuffer, pCurrentHeader, H263_REQUIRE_HEADER_SIZE_BYTES);
    frameSize = H263_BUFFER_SIZE - H263_REQUIRE_HEADER_SIZE_BYTES;
    // Read 1 frame and the next frame's header from the file.
    // For the first frame, only the first frame's header is returned.
    // For the last frame, only the last frame's data is returned.
    if (! LoadNextH263Object(inputFileHandle, pFrameBuffer, &frameSize,
          H263_REQUIRE_HEADER_SIZE_BYTES - H263_STARTCODE_SIZE_BYTES,
          &pCurrentHeader))
      break; // Fatal error ...

    if (pCurrentHeader) {
      // Parse the returned frame header (if any)
      if (!ParseShortHeader(pCurrentHeader, &nextInfo))
        break; // Fatal error
      trDifference = GetTRDifference(nextInfo.tr, currentInfo.tr);
    } else {
      // This is the last frame ... we have to fake the trDifference ...
      trDifference = 1;
      // No header data has been read at this iteration, so we have to manually
      // add the frame's header we read at the previous iteration.
      // Note that LoadNextH263Object returns the number of bytes read, which
      // are the current frame's data and the next frame's header
      frameSize += H263_REQUIRE_HEADER_SIZE_BYTES;
      // There is no need for the next iteration ...
      stay = false;
    }

    // If this is the first iteration ...
    if (currentInfo.width == 0) {
      // If we have more data than just the header
      if ((frameSize > H263_REQUIRE_HEADER_SIZE_BYTES) ||
          !pCurrentHeader)  // Or no header at all
        break;     // Fatal error
      else
        continue;  // We have only the first frame's header ...
    }

    if (trackId == MP4_INVALID_TRACK_ID) {
      //  If a track has not been added yet, add the track to the file.
      trackId = MP4AddH263VideoTrack(outputFileHandle, H263_TIMESCALE,
          0, currentInfo.width, currentInfo.height,
          h263Level, h263Profile, 0, 0);
      if (trackId == MP4_INVALID_TRACK_ID)
        break; // Fatal error
    }

    // calculate the current frame duration
    currentDuration = CalculateDuration(trDifference);
    // Write the current frame to the file.
    if (!MP4WriteSample(outputFileHandle, trackId, frameBuffer, frameSize,
          currentDuration, 0, currentInfo.isSyncFrame))
      break; // Fatal error

    // Accumulate the frame's size and duration for avgBitrate calculation
    totalDuration += currentDuration;
    totalBytes += frameSize;
    //  If needed, recalculate bitrate information
    if (h263Bitrates)
      GetMaxBitrate(&maxBitrateCtx, frameSize, prevTrDifference);
    prevTrDifference = trDifference;
  } // while (stay)

  // If this is the last frame,
  if (!stay) {
    // If needed and possible, update bitrate information in the file
    if (h263Bitrates && totalDuration) {
      avgBitrate = (totalBytes * 8 * H263_TIMESCALE) / totalDuration;
      if (cbrTolerance == 0)
        cbrTolerance = H263_DEFAULT_CBR_TOLERANCE;
      // Same as: if (maxBitrate / avgBitrate > (cbrTolerance + 100) / 100.0)
      if (maxBitrateCtx.maxBitrate * 100 > (cbrTolerance + 100) * avgBitrate)
        avgBitrate = 0;
      MP4SetH263Bitrates(outputFileHandle, trackId,
          avgBitrate, maxBitrateCtx.maxBitrate);
    }
    // Return the newly added track ID
    return trackId;
  }

  // If we got to here... something went wrong ...
  fprintf(stderr,
    "%s: Could not parse input file, invalid video stream?\n", ProgName);
  // Upon failure, delete the newly added track if it has been added
  if (trackId != MP4_INVALID_TRACK_ID) {
    MP4DeleteTrack(outputFileHandle, trackId);
  }
  return MP4_INVALID_TRACK_ID;
}

/ *
 * LoadNextH263Object - service routine that reads a single frame from the input
 * file. It shall fill the input buffer with data up until - and including - the
 * next start code and shall report back both the number of bytes read and a
 * pointer to the next start code. The first call to this function shall only
 * yield a pointer with 0 data bytes and the last call to this function shall
 * only yield data bytes with a NULL pointer as the next header.
 *
 * TODO: This function only supports valid bit streams. Upon error, it fails
 * without the possibility to recover. A Better idea would be to skip frames
 * until a parsable frame is read from the file.
 *
 * Parameters:
 *      inputFileHandle - The handle of the input file
 *      frameBuffer - buffer where to place read data
 *      frameBufferSize - in/out parameter indicating the size of the buffer on
 *                          entry and the number of bytes copied to the buffer upon
 *                          return
 *      additionalBytesNeeded - indicates how many additional bytes are to be read
 *                          from the next frame's header (over the 3 bytes that
 *                          are already read).
 *                          NOTE: This number MUST be > 0
 *      ppNextHeader - output parameter that upon return points to the location
 *                     of the next frame's head in the buffer
 * Outputs:
 *      This function returns two pieces of information:
 *      1. The total number of bytes read.
 *      2. A Pointer to the header of the next frame. This pointer shall be NULL
 *      for the last frame read.
 * /
static int LoadNextH263Object(  FILE           *inputFileHandle,
                                u_int8_t       *frameBuffer,
                                u_int32_t      *frameBufferSize,
                                u_int32_t       additionalBytesNeeded,
                                u_int8_t      **ppNextHeader)
{
  // This table and the following loop implements a state machine enabling
  // us to read bytes from the file untill (and inclusing) the requested
  // start code (00 00 8X) is found
  int8_t        row = 0;
  u_int8_t     *bufferStart = frameBuffer;
  // The buffer end which will allow the loop to leave place for
  // the additionalBytesNeeded
  u_int8_t     *bufferEnd = frameBuffer + *frameBufferSize -
                                              additionalBytesNeeded - 1;

  // Initialize the states array, if it hasn't been initialized yet...
  if (!states[0][0]) {
    // One 00 was read
    states[0][0] = 1;
    // Two sequential 0x00 ware read
    states[1][0] = states[2][0] = 2;
    // A full start code was read
    states[2][128] = states[2][129] = states[2][130] = states[2][131] = -1;
  }

  // Read data from file into the output buffer until either a start code
  // is found, or the end of file has been reached.
  do {
    if (fread(frameBuffer, 1, 1, inputFileHandle) != 1){
      // EOF or other error before we got a start code
      *ppNextHeader = NULL;
      *frameBufferSize = frameBuffer - bufferStart;
      return 1;
    }
  } while ((frameBuffer < bufferEnd) &&                    // We have place in the buffer
           ((row = states[row][*(frameBuffer++)]) != -1)); // Start code was not found
  if (row != -1) {
    fprintf(stderr, "%s: Buffer too small (%u)\n",
            ProgName, bufferEnd - bufferStart + additionalBytesNeeded);
    return 0;
  }

  // Cool ... now we have a start code
  *ppNextHeader = frameBuffer - H263_STARTCODE_SIZE_BYTES;
  *frameBufferSize = frameBuffer - bufferStart + additionalBytesNeeded;

  // Now we just have to read the additionalBytesNeeded
  if(fread(frameBuffer, additionalBytesNeeded, 1, inputFileHandle) != 1) {
    /// We got a start code but can't read additionalBytesNeeded ... that's a fatal error
    fprintf(stderr, "%s: Invalid H263 bitstream\n", ProgName);
    return 0;
  }

  return 1;
}


/ *
 * ParseShortHeader - service routine that accepts a buffer containing a frame
 * header and extracts relevant codec information from it.
 *
 * NOTE: the first bit in the following commnets is 0 (zero).
 *
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |      PSC (Picture Start Code=22 bits)     |  (TR=8 bits)  |   >
 *      |0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0|               |1 0>
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      <   (PTYPE=13 bits)   |
 *      <. . .|(FMT)|Z|. . . .|
 *      +-+-+-+-+-+-+-+-+-+-+-+
 *      -> PTYPE.FMT contains a width/height identification
 *      -> PTYPE.Z   is 1 for P-Frames, 0 for I-Frames
 *      Note: When FMT is 111, there is an extended PTYPE...
 *
 * Inputs:
 *      headerBuffer - pointer to the current header buffer
 *      outputInfoStruct - pointer to the structure receiving the data
 * Outputs:
 *      This function returns a structure of important codec-specific
 *      information (The Temporal Reference bits, width & height of the current
 *      frame and the sync - or "frame type" - bit. It reports success or
 *      failure to the calling function.
 * /
static bool ParseShortHeader(   u_int8_t       *headerBuffer,
                                H263INFO       *outputInfoStruct)
{
  u_int8_t fmt = 0;
  // Extract temporal reference (TR) from the buffer (bits 22-29 inclusive)
  outputInfoStruct->tr  = (headerBuffer[2] << 6) & 0xC0; // 2 LS bits out of the 3rd byte
  outputInfoStruct->tr |= (headerBuffer[3] >> 2) & 0x3F; // 6 MS bits out of the 4th byte
  // Extract the FMT part of PTYPE from the buffer (bits 35-37 inclusive)
  fmt = (headerBuffer[4] >> 2) & 0x07; // bits 3-5 ouf of the 5th byte
  // If PTYPE is not supported, return a failure notice to the calling function
  // FIXME: PLUSPTYPE is not supported
   if (fmt == 0x07) {
    return false;
  }
  // If PTYPE is supported, calculate the current width and height according to
  // a predefined table
  if (!GetWidthAndHeight(fmt, &(outputInfoStruct->width),
                              &(outputInfoStruct->height))) {
    return false;
  }
  // Extract the frame-type bit, which is the 9th bit of PTYPE (bit 38)
  outputInfoStruct->isSyncFrame = !(headerBuffer[4] & 0x02);

  return true;
}

/ *
 * GetMaxBitrate- service routine that accepts frame information and
 * derives bitrate information from it. This function uses a sliding window
 * technique to calculate the maximum bitrates in any window of 1 second
 * inside the file.
 * The sliding window is implemented with a table of bitrates for the last
 * second (30 entries - one entry per TR unit).
 *
 * Inputs:
 *      ctx - context for this function
 *      frameSize - the size of the current frame in bytes
 *      frameTRDiff - the "duration" of the frame in TR units
 * Outputs:
 *      This function returns the up-to-date maximum bitrate
 * /
static void GetMaxBitrate(      MaxBitrate_CTX *ctx,
                                u_int32_t       frameSize,
                                u_int8_t        frameTRDiff)
{
  if (frameTRDiff == 0)
    return;

  // Calculate the current frame's bitrate as bits per TR unit (round the result
  // upwards)
  u_int32_t frameBitrate = frameSize * 8 / frameTRDiff + 1;

  // for each TRdiff received,
  while (frameTRDiff--) {
    // Subtract the oldest bitrate entry from the current bitrate
    ctx->windowBitrate -= ctx->bitrateTable[ctx->tableIndex];
     // Update the oldest bitrate entry with the current frame's bitrate
    ctx->bitrateTable[ctx->tableIndex] = frameBitrate;
    // Add the current frame's bitrate to the current bitrate
    ctx->windowBitrate += frameBitrate;
    // Check if we have a new maximum bitrate
    if (ctx->windowBitrate > ctx->maxBitrate) {
      ctx->maxBitrate = ctx->windowBitrate;
    }
    // Advance the table index
    ctx->tableIndex = (ctx->tableIndex + 1) %
        // Wrapping around the bitrateTable size
        ( sizeof(ctx->bitrateTable) / sizeof(ctx->bitrateTable[0]) );
  }
}

/ *
 * CalculateDuration - service routine that calculates the current frame's
 * duration in milli-seconds using it's duration in TR units.
 *  - In order not to accumulate the calculation error, we are using the TR
 * duration to calculate the current and the next frame's presentation time in
 * milli-seconds.
 *
 * Inputs:
 *      trDiff - The current frame's duration in TR units
 * Outputs:
 *      The current frame's duration in milli-seconds
 * /
static MP4Duration CalculateDuration(u_int8_t   trDiff)
{
  static u_int32_t    nextTR    = 0;   // The next frame's presentation time in TR units
  static MP4Duration  currentPT = 0;   // The current frame's presentation time in milli-seconds
  MP4Duration         nextPT;          // The next frame's presentation time in milli-seconds
  MP4Duration         duration;        // The current frame's duration in milli-seconds

  nextTR += trDiff;
  // Calculate the next frame's presentation time, in milli-seconds
  nextPT = (nextTR * 1001) / H263_BASIC_FRAME_RATE;
  // The frame's duration is the difference between the next presentation
  // time and the current presentation time.
  duration = nextPT - currentPT;
  // "Remember" the next presentation time for the next time this function is
  // called
  currentPT = nextPT;

  return duration;
}

static bool GetWidthAndHeight(  u_int8_t        fmt,
                                u_int16_t      *width,
                                u_int16_t      *height)
{
  // The 'fmt' corresponds to bits 5-7 of the PTYPE
  static struct {
    u_int16_t width;
    u_int16_t height;
  } dimensionsTable[8] = {
    { 0,    0 },      // 000 - 0 - forbidden, generates an error
    { 128,  96 },     // 001 - 1 - Sub QCIF
    { 176,  144 },    // 010 - 2 - QCIF
    { 352,  288 },    // 011 - 3 - CIF
    { 704,  576 },    // 100 - 4 - 4CIF
    { 1409, 1152 },   // 101 - 5 - 16CIF
    { 0,    0 },      // 110 - 6 - reserved, generates an error
    { 0,    0 }       // 111 - 7 - extended, not supported by profile 0
  };

  if (fmt > 7)
    return false;

  *width  = dimensionsTable[fmt].width;
  *height = dimensionsTable[fmt].height;

  if (*width  == 0)
    return false;

  return true;
}

static u_int8_t GetTRDifference(u_int8_t        nextTR,
                                u_int8_t        currentTR)
{
  if (currentTR > nextTR) {
    // Wrap around 255...
    return nextTR + (256 - currentTR);
  } else {
    return nextTR - currentTR;
  }
}

*/

