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
// A filter for converting one or more MPEG Elementary Streams
// to a MPEG-2 Transport Stream
// C++ header

#ifndef _MPEG2_TRANSPORT_STREAM_FROM_ES_SOURCE_HH
#define _MPEG2_TRANSPORT_STREAM_FROM_ES_SOURCE_HH

#ifndef _MPEG2_TRANSPORT_STREAM_MULTIPLEXOR_HH
#include "MPEG2TransportStreamMultiplexor.hh"
#endif

class MPEG2TransportStreamFromESSource: public MPEG2TransportStreamMultiplexor {
public:
  static MPEG2TransportStreamFromESSource* createNew(UsageEnvironment& env);

  void addNewVideoSource(FramedSource* inputSource, int mpegVersion);
      // Note: For MPEG-4 video, set "mpegVersion" to 4; for H.264 video, set "mpegVersion"to 5.
  void addNewAudioSource(FramedSource* inputSource, int mpegVersion);

protected:
  MPEG2TransportStreamFromESSource(UsageEnvironment& env);
      // called only by createNew()
  virtual ~MPEG2TransportStreamFromESSource();

private:
  // Redefined virtual functions:
  virtual void doStopGettingFrames();
  virtual void awaitNewBuffer(unsigned char* oldBuffer);

private:
  void addNewInputSource(FramedSource* inputSource,
			 u_int8_t streamId, int mpegVersion);
  // used to implement addNew*Source() above

private:
  friend class InputESSourceRecord;
  class InputESSourceRecord* fInputSources;
  unsigned fVideoSourceCounter, fAudioSourceCounter;
};

#endif
