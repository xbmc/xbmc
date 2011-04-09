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
// An abstract parser for MPEG video streams
// Implementation

#include "MPEGVideoStreamParser.hh"

MPEGVideoStreamParser
::MPEGVideoStreamParser(MPEGVideoStreamFramer* usingSource,
			FramedSource* inputSource)
  : StreamParser(inputSource, FramedSource::handleClosure, usingSource,
		 &MPEGVideoStreamFramer::continueReadProcessing, usingSource),
  fUsingSource(usingSource) {
}

MPEGVideoStreamParser::~MPEGVideoStreamParser() {
}

void MPEGVideoStreamParser::restoreSavedParserState() {
  StreamParser::restoreSavedParserState();
  fTo = fSavedTo;
  fNumTruncatedBytes = fSavedNumTruncatedBytes;
}

void MPEGVideoStreamParser::setParseState() {
  fSavedTo = fTo;
  fSavedNumTruncatedBytes = fNumTruncatedBytes;
  saveParserState();
}

void MPEGVideoStreamParser::registerReadInterest(unsigned char* to,
						 unsigned maxSize) {
  fStartOfFrame = fTo = fSavedTo = to;
  fLimit = to + maxSize;
  fNumTruncatedBytes = fSavedNumTruncatedBytes = 0;
}
