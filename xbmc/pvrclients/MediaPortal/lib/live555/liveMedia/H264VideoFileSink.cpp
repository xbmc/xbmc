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
// H.264 Video File sinks
// Implementation

#include "H264VideoFileSink.hh"
#include "OutputFile.hh"

////////// H264VideoFileSink //////////

H264VideoFileSink
::H264VideoFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		   char const* perFrameFileNamePrefix)
  : FileSink(env, fid, bufferSize, perFrameFileNamePrefix) {
}

H264VideoFileSink::~H264VideoFileSink() {
}

H264VideoFileSink*
H264VideoFileSink::createNew(UsageEnvironment& env, char const* fileName,
			    unsigned bufferSize, Boolean oneFilePerFrame) {
  do {
    FILE* fid;
    char const* perFrameFileNamePrefix;
    if (oneFilePerFrame) {
      // Create the fid for each frame
      fid = NULL;
      perFrameFileNamePrefix = fileName;
    } else {
      // Normal case: create the fid once
      fid = OpenOutputFile(env, fileName);
      if (fid == NULL) break;
      perFrameFileNamePrefix = NULL;
    }

    return new H264VideoFileSink(env, fid, bufferSize, perFrameFileNamePrefix);
  } while (0);

  return NULL;
}

Boolean H264VideoFileSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Just return true, should be checking for H.264 video streams though
    return True;
}

void H264VideoFileSink::afterGettingFrame1(unsigned frameSize,
					  struct timeval presentationTime) {
  unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};
  addData(start_code, 4, presentationTime);

  // Call the parent class to complete the normal file write with the input data:
  FileSink::afterGettingFrame1(frameSize, presentationTime);
}
