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
//

#include "H263plusVideoStreamFramer.hh"
#include "H263plusVideoStreamParser.hh"

#include <string.h>
#include <GroupsockHelper.hh>


///////////////////////////////////////////////////////////////////////////////
////////// H263plusVideoStreamFramer implementation //////////
//public///////////////////////////////////////////////////////////////////////
H263plusVideoStreamFramer* H263plusVideoStreamFramer::createNew(
                                                         UsageEnvironment& env,
                                                         FramedSource* inputSource)
{
   // Need to add source type checking here???  #####
   H263plusVideoStreamFramer* fr;
   fr = new H263plusVideoStreamFramer(env, inputSource);
   return fr;
}


///////////////////////////////////////////////////////////////////////////////
H263plusVideoStreamFramer::H263plusVideoStreamFramer(
                              UsageEnvironment& env,
                              FramedSource* inputSource,
                              Boolean createParser)
                              : FramedFilter(env, inputSource),
                                fFrameRate(0.0), // until we learn otherwise
                                fPictureEndMarker(False)
{
   // Use the current wallclock time as the base 'presentation time':
   gettimeofday(&fPresentationTimeBase, NULL);
   fParser = createParser ? new H263plusVideoStreamParser(this, inputSource) : NULL;
}

///////////////////////////////////////////////////////////////////////////////
H263plusVideoStreamFramer::~H263plusVideoStreamFramer()
{
   delete   fParser;
}


///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
static struct timeval firstPT;
#endif


///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamFramer::doGetNextFrame()
{
  fParser->registerReadInterest(fTo, fMaxSize);
  continueReadProcessing();
}


///////////////////////////////////////////////////////////////////////////////
Boolean H263plusVideoStreamFramer::isH263plusVideoStreamFramer() const
{
  return True;
}

///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamFramer::continueReadProcessing(
                                   void* clientData,
                                   unsigned char* /*ptr*/, unsigned /*size*/,
                                   struct timeval /*presentationTime*/)
{
   H263plusVideoStreamFramer* framer = (H263plusVideoStreamFramer*)clientData;
   framer->continueReadProcessing();
}

///////////////////////////////////////////////////////////////////////////////
void H263plusVideoStreamFramer::continueReadProcessing()
{
   unsigned acquiredFrameSize;

   u_int64_t frameDuration;  // in ms

   acquiredFrameSize = fParser->parse(frameDuration);
// Calculate some average bitrate information (to be adapted)
//	avgBitrate = (totalBytes * 8 * H263_TIMESCALE) / totalDuration;

   if (acquiredFrameSize > 0) {
      // We were able to acquire a frame from the input.
      // It has already been copied to the reader's space.
      fFrameSize = acquiredFrameSize;
//    fNumTruncatedBytes = fParser->numTruncatedBytes(); // not needed so far

      fFrameRate = frameDuration == 0 ? 0.0 : 1000./(long)frameDuration;

      // Compute "fPresentationTime"
      if (acquiredFrameSize == 5) // first frame
         fPresentationTime = fPresentationTimeBase;
      else
         fPresentationTime.tv_usec += (long) frameDuration*1000;

      while (fPresentationTime.tv_usec >= 1000000) {
         fPresentationTime.tv_usec -= 1000000;
         ++fPresentationTime.tv_sec;
      }

      // Compute "fDurationInMicroseconds"
      fDurationInMicroseconds = (unsigned int) frameDuration*1000;;

      // Call our own 'after getting' function.  Because we're not a 'leaf'
      // source, we can call this directly, without risking infinite recursion.
      afterGetting(this);
   } else {
      // We were unable to parse a complete frame from the input, because:
      // - we had to read more data from the source stream, or
      // - the source stream has ended.
   }
}
