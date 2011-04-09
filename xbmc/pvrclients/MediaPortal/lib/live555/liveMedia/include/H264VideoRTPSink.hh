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
// RTP sink for H.264 video (RFC 3984)
// C++ header

#ifndef _H264_VIDEO_RTP_SINK_HH
#define _H264_VIDEO_RTP_SINK_HH

#ifndef _VIDEO_RTP_SINK_HH
#include "VideoRTPSink.hh"
#endif
#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class H264FUAFragmenter;

class H264VideoRTPSink: public VideoRTPSink {
public:
  static H264VideoRTPSink* createNew(UsageEnvironment& env,
				     Groupsock* RTPgs,
				     unsigned char rtpPayloadFormat,
				     unsigned profile_level_id,
				     char const* sprop_parameter_sets_str);

protected:
  H264VideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		   unsigned char rtpPayloadFormat,
		   unsigned profile_level_id,
		   char const* sprop_parameter_sets_str);
	// called only by createNew()

  virtual ~H264VideoRTPSink();

protected: // redefined virtual functions:
  virtual char const* auxSDPLine();

private: // redefined virtual functions:
  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);
  virtual Boolean continuePlaying();
  virtual void stopPlaying();
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
						 unsigned numBytesInFrame) const;

protected:
  H264FUAFragmenter* fOurFragmenter;

private:
  char* fFmtpSDPLine;
};


////////// H264FUAFragmenter definition //////////

// Because of the ideosyncracies of the H.264 RTP payload format, we implement
// "H264VideoRTPSink" using a separate "H264FUAFragmenter" class that delivers,
// to the "H264VideoRTPSink", only fragments that will fit within an outgoing
// RTP packet.  I.e., we implement fragmentation in this separate "H264FUAFragmenter"
// class, rather than in "H264VideoRTPSink".
// (Note: This class should be used only by "H264VideoRTPSink", or a subclass.)

class H264FUAFragmenter: public FramedFilter {
public:
  H264FUAFragmenter(UsageEnvironment& env, FramedSource* inputSource,
		    unsigned inputBufferMax, unsigned maxOutputPacketSize);
  virtual ~H264FUAFragmenter();

  Boolean lastFragmentCompletedNALUnit() const { return fLastFragmentCompletedNALUnit; }

private: // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);

private:
  unsigned fInputBufferSize;
  unsigned fMaxOutputPacketSize;
  unsigned char* fInputBuffer;
  unsigned fNumValidDataBytes;
  unsigned fCurDataOffset;
  unsigned fSaveNumTruncatedBytes;
  Boolean fLastFragmentCompletedNALUnit;
};


#endif
