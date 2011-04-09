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
// AMR Audio File sinks
// Implementation

#include "AMRAudioFileSink.hh"
#include "AMRAudioSource.hh"
#include "OutputFile.hh"

////////// AMRAudioFileSink //////////

AMRAudioFileSink
::AMRAudioFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		   char const* perFrameFileNamePrefix)
  : FileSink(env, fid, bufferSize, perFrameFileNamePrefix),
    fHaveWrittenHeader(False) {
}

AMRAudioFileSink::~AMRAudioFileSink() {
}

AMRAudioFileSink*
AMRAudioFileSink::createNew(UsageEnvironment& env, char const* fileName,
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

    return new AMRAudioFileSink(env, fid, bufferSize, perFrameFileNamePrefix);
  } while (0);

  return NULL;
}

Boolean AMRAudioFileSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // The input source must be a AMR Audio source:
  return source.isAMRAudioSource();
}

void AMRAudioFileSink::afterGettingFrame1(unsigned frameSize,
					  struct timeval presentationTime) {
  AMRAudioSource* source = (AMRAudioSource*)fSource;
  if (source == NULL) return; // sanity check

  if (!fHaveWrittenHeader && fPerFrameFileNameBuffer == NULL) {
    // Output the appropriate AMR header to the start of the file.
    // This header is defined in RFC 3267, section 5.
    // (However, we don't do this if we're creating one file per frame.)
    char headerBuffer[100];
    sprintf(headerBuffer, "#!AMR%s%s\n",
	    source->isWideband() ? "-WB" : "",
	    source->numChannels() > 1 ? "_MC1.0" : "");
    unsigned headerLength = strlen(headerBuffer);
    if (source->numChannels() > 1) {
      // Also add a 32-bit channel description field:
      headerBuffer[headerLength++] = 0;
      headerBuffer[headerLength++] = 0;
      headerBuffer[headerLength++] = 0;
      headerBuffer[headerLength++] = source->numChannels();
    }

    addData((unsigned char*)headerBuffer, headerLength, presentationTime);
  }
  fHaveWrittenHeader = True;

  // Add the 1-byte header, before writing the file data proper:
  // (Again, we don't do this if we're creating one file per frame.)
  if (fPerFrameFileNameBuffer == NULL) {
    u_int8_t frameHeader = source->lastFrameHeader();
    addData(&frameHeader, 1, presentationTime);
  }

  // Call the parent class to complete the normal file write with the input data:
  FileSink::afterGettingFrame1(frameSize, presentationTime);
}
