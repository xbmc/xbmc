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
// A template for a MediaSource encapsulating an audio/video input device
// Implementation

#include "DeviceSource.hh"

DeviceSource*
DeviceSource::createNew(UsageEnvironment& env,
			DeviceParameters params) {
  return new DeviceSource(env, params);
}

DeviceSource::DeviceSource(UsageEnvironment& env,
			   DeviceParameters params)
  : FramedSource(env), fParams(params) {
  // Any initialization of the device would be done here
}

DeviceSource::~DeviceSource() {
}

void DeviceSource::doGetNextFrame() {
  // Arrange here for our "deliverFrame" member function to be called
  // when the next frame of data becomes available from the device.
  // This must be done in a non-blocking fashion - i.e., so that we
  // return immediately from this function even if no data is
  // currently available.
  //
  // If the device can be implemented as a readable socket, then one easy
  // way to do this is using a call to
  //     envir().taskScheduler().turnOnBackgroundReadHandling( ... )
  // (See examples of this call in the "liveMedia" directory.)

  // If, for some reason, the source device stops being readable
  // (e.g., it gets closed), then you do the following:
  if (0 /* the source stops being readable */) {
    handleClosure(this);
    return;
  }
}

void DeviceSource::deliverFrame() {
  // This would be called when new frame data is available from the device.
  // This function should deliver the next frame of data from the device,
  // using the following parameters (class members):
  // 'in' parameters (these should *not* be modified by this function):
  //     fTo: The frame data is copied to this address.
  //         (Note that the variable "fTo" is *not* modified.  Instead,
  //          the frame data is copied to the address pointed to by "fTo".)
  //     fMaxSize: This is the maximum number of bytes that can be copied
  //         (If the actual frame is larger than this, then it should
  //          be truncated, and "fNumTruncatedBytes" set accordingly.)
  // 'out' parameters (these are modified by this function):
  //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
  //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
  //         bigger than "fMaxSize", in which case it's set to the number of bytes
  //         that have been omitted.
  //     fPresentationTime: Should be set to the frame's presentation time
  //         (seconds, microseconds).
  //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
  if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

  // Deliver the data here:

  // After delivering the data, inform the reader that it is now available:
  FramedSource::afterGetting(this);
}
