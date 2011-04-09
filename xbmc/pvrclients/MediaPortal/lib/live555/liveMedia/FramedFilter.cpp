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
// Framed Filters
// Implementation

#include "FramedFilter.hh"

////////// FramedFilter //////////
#include <string.h>

FramedFilter::FramedFilter(UsageEnvironment& env,
			   FramedSource* inputSource)
  : FramedSource(env),
    fInputSource(inputSource) {
}

FramedFilter::~FramedFilter() {
  Medium::close(fInputSource);
}

// Default implementations of needed virtual functions.  These merely
// call the same function in the input source - i.e., act like a 'null filter

char const* FramedFilter::MIMEtype() const {
  if (fInputSource == NULL) return "";

  return fInputSource->MIMEtype();
}

void FramedFilter::getAttributes() const {
  if (fInputSource != NULL) fInputSource->getAttributes();
}

void FramedFilter::doStopGettingFrames() {
  if (fInputSource != NULL) fInputSource->stopGettingFrames();
}
