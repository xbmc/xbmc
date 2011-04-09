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
// Any source that feeds into a "H264VideoRTPSink" must be of this class.
// This is a virtual base class; subclasses must implement the
// "currentNALUnitEndsAccessUnit()" virtual function.
// Implementation

#include "H264VideoStreamFramer.hh"

H264VideoStreamFramer::H264VideoStreamFramer(UsageEnvironment& env, FramedSource* inputSource)
  : FramedFilter(env, inputSource) {
}

H264VideoStreamFramer::~H264VideoStreamFramer() {
}

Boolean H264VideoStreamFramer::isH264VideoStreamFramer() const {
  return True;
}
