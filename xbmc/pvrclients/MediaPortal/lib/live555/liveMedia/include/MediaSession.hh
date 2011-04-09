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
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// (This data structure is used for media *receivers* - i.e., clients.
//  For media streamers, use "ServerMediaSession" instead.)
// C++ header

#ifndef _MEDIA_SESSION_HH
#define _MEDIA_SESSION_HH

#ifndef _RTCP_HH
#include "RTCP.hh"
#endif

class MediaSubsession; // forward

class MediaSession: public Medium {
public:
  static MediaSession* createNew(UsageEnvironment& env,
				 char const* sdpDescription);

  static Boolean lookupByName(UsageEnvironment& env, char const* sourceName,
			      MediaSession*& resultSession);

  Boolean hasSubsessions() const { return fSubsessionsHead != NULL; }
  double& playStartTime() { return fMaxPlayStartTime; }
  double& playEndTime() { return fMaxPlayEndTime; }
  char* connectionEndpointName() const { return fConnectionEndpointName; }
  char const* CNAME() const { return fCNAME; }
  struct in_addr const& sourceFilterAddr() const { return fSourceFilterAddr; }
  float& scale() { return fScale; }
  char* mediaSessionType() const { return fMediaSessionType; }
  char* sessionName() const { return fSessionName; }
  char* sessionDescription() const { return fSessionDescription; }
  char const* controlPath() const { return fControlPath; }

  Boolean initiateByMediaType(char const* mimeType,
			      MediaSubsession*& resultSubsession,
			      int useSpecialRTPoffset = -1);
      // Initiates the first subsession with the specified MIME type
      // Returns the resulting subsession, or 'multi source' (not both)

#ifdef SUPPORT_REAL_RTSP
  // Attributes specific to RealNetworks streams:
  Boolean isRealNetworksRDT;
  unsigned fRealFlags;
  unsigned char* fRealTitle; unsigned fRealTitleSize;
  unsigned char* fRealAuthor; unsigned fRealAuthorSize;
  unsigned char* fRealCopyright; unsigned fRealCopyrightSize;
  unsigned char* fRealAbstract; unsigned fRealAbstractSize;
#endif

protected: // redefined virtual functions
  virtual Boolean isMediaSession() const;

protected:
  MediaSession(UsageEnvironment& env);
      // called only by createNew();
  virtual ~MediaSession();

  Boolean initializeWithSDP(char const* sdpDescription);
  Boolean parseSDPLine(char const* input, char const*& nextLine);
  Boolean parseSDPLine_s(char const* sdpLine);
  Boolean parseSDPLine_i(char const* sdpLine);
  Boolean parseSDPLine_c(char const* sdpLine);
  Boolean parseSDPAttribute_type(char const* sdpLine);
  Boolean parseSDPAttribute_control(char const* sdpLine);
  Boolean parseSDPAttribute_range(char const* sdpLine);
  Boolean parseSDPAttribute_source_filter(char const* sdpLine);

  static char* lookupPayloadFormat(unsigned char rtpPayloadType,
				   unsigned& rtpTimestampFrequency,
				   unsigned& numChannels);
  static unsigned guessRTPTimestampFrequency(char const* mediumName,
					     char const* codecName);

protected:
  friend class MediaSubsessionIterator;
  char* fCNAME; // used for RTCP

  // Linkage fields:
  MediaSubsession* fSubsessionsHead;
  MediaSubsession* fSubsessionsTail;

  // Fields set from a SDP description:
  char* fConnectionEndpointName;
  double fMaxPlayStartTime;
  double fMaxPlayEndTime;
  struct in_addr fSourceFilterAddr; // used for SSM
  float fScale; // set from a RTSP "Scale:" header
  char* fMediaSessionType; // holds a=type value
  char* fSessionName; // holds s=<session name> value
  char* fSessionDescription; // holds i=<session description> value
  char* fControlPath; // holds optional a=control: string
};


class MediaSubsessionIterator {
public:
  MediaSubsessionIterator(MediaSession& session);
  virtual ~MediaSubsessionIterator();

  MediaSubsession* next(); // NULL if none
  void reset();

private:
  MediaSession& fOurSession;
  MediaSubsession* fNextPtr;
};


class MediaSubsession {
public:
  MediaSession& parentSession() { return fParent; }
  MediaSession const& parentSession() const { return fParent; }

  unsigned short clientPortNum() const { return fClientPortNum; }
  unsigned char rtpPayloadFormat() const { return fRTPPayloadFormat; }
  char const* savedSDPLines() const { return fSavedSDPLines; }
  char const* mediumName() const { return fMediumName; }
  char const* codecName() const { return fCodecName; }
  char const* protocolName() const { return fProtocolName; }
  char const* controlPath() const { return fControlPath; }
  Boolean isSSM() const { return fSourceFilterAddr.s_addr != 0; }

  unsigned short videoWidth() const { return fVideoWidth; }
  unsigned short videoHeight() const { return fVideoHeight; }
  unsigned videoFPS() const { return fVideoFPS; }
  unsigned numChannels() const { return fNumChannels; }
  float& scale() { return fScale; }

  RTPSource* rtpSource() { return fRTPSource; }
  RTCPInstance* rtcpInstance() { return fRTCPInstance; }
  unsigned rtpTimestampFrequency() const { return fRTPTimestampFrequency; }
  FramedSource* readSource() { return fReadSource; }
    // This is the source that client sinks read from.  It is usually
    // (but not necessarily) the same as "rtpSource()"

  double playStartTime() const;
  double playEndTime() const;
  // Used only to set the local fields:
  double& _playStartTime() { return fPlayStartTime; }
  double& _playEndTime() { return fPlayEndTime; }

  Boolean initiate(int useSpecialRTPoffset = -1);
      // Creates a "RTPSource" for this subsession. (Has no effect if it's
      // already been created.)  Returns True iff this succeeds.
  void deInitiate(); // Destroys any previously created RTPSource
  Boolean setClientPortNum(unsigned short portNum);
      // Sets the preferred client port number that any "RTPSource" for
      // this subsession would use.  (By default, the client port number
      // is gotten from the original SDP description, or - if the SDP
      // description does not specfy a client port number - an ephemeral
      // (even) port number is chosen.)  This routine should *not* be
      // called after initiate().
  char*& connectionEndpointName() { return fConnectionEndpointName; }
  char const* connectionEndpointName() const {
    return fConnectionEndpointName;
  }

  // Various parameters set in "a=fmtp:" SDP lines:
  unsigned fmtp_auxiliarydatasizelength() const { return fAuxiliarydatasizelength; }
  unsigned fmtp_constantduration() const { return fConstantduration; }
  unsigned fmtp_constantsize() const { return fConstantsize; }
  unsigned fmtp_crc() const { return fCRC; }
  unsigned fmtp_ctsdeltalength() const { return fCtsdeltalength; }
  unsigned fmtp_de_interleavebuffersize() const { return fDe_interleavebuffersize; }
  unsigned fmtp_dtsdeltalength() const { return fDtsdeltalength; }
  unsigned fmtp_indexdeltalength() const { return fIndexdeltalength; }
  unsigned fmtp_indexlength() const { return fIndexlength; }
  unsigned fmtp_interleaving() const { return fInterleaving; }
  unsigned fmtp_maxdisplacement() const { return fMaxdisplacement; }
  unsigned fmtp_objecttype() const { return fObjecttype; }
  unsigned fmtp_octetalign() const { return fOctetalign; }
  unsigned fmtp_profile_level_id() const { return fProfile_level_id; }
  unsigned fmtp_robustsorting() const { return fRobustsorting; }
  unsigned fmtp_sizelength() const { return fSizelength; }
  unsigned fmtp_streamstateindication() const { return fStreamstateindication; }
  unsigned fmtp_streamtype() const { return fStreamtype; }
  Boolean fmtp_cpresent() const { return fCpresent; }
  Boolean fmtp_randomaccessindication() const { return fRandomaccessindication; }
  char const* fmtp_config() const { return fConfig; }
  char const* fmtp_mode() const { return fMode; }
  char const* fmtp_spropparametersets() const { return fSpropParameterSets; }

  netAddressBits connectionEndpointAddress() const;
      // Converts "fConnectionEndpointName" to an address (or 0 if unknown)
  void setDestinations(netAddressBits defaultDestAddress);
      // Uses "fConnectionEndpointName" and "serverPortNum" to set
      // the destination address and port of the RTP and RTCP objects.
      // This is typically called by RTSP clients after doing "SETUP".

  // Public fields that external callers can use to keep state.
  // (They are responsible for all storage management on these fields)
  char const* sessionId; // used by RTSP
  unsigned short serverPortNum; // in host byte order (used by RTSP)
  unsigned char rtpChannelId, rtcpChannelId; // used by RTSP (for RTP/TCP)
  MediaSink* sink; // callers can use this to keep track of who's playing us
  void* miscPtr; // callers can use this for whatever they want

  // Parameters set from a RTSP "RTP-Info:" header:
  struct {
    u_int16_t seqNum;
    u_int32_t timestamp;
    Boolean infoIsNew; // not part of the RTSP header; instead, set whenever this struct is filled in
  } rtpInfo;

  double getNormalPlayTime(struct timeval const& presentationTime);
  // Computes the stream's "Normal Play Time" (NPT) from the given "presentationTime".
  // (For the definition of "Normal Play Time", see RFC 2326, section 3.6.)
  // This function is useful only if the "rtpInfo" structure was previously filled in
  // (e.g., by a "RTP-Info:" header in a RTSP response).
  // Also, for this function to work properly, the RTP stream's presentation times must (eventually) be
  // synchronized via RTCP.

#ifdef SUPPORT_REAL_RTSP
  // Attributes specific to RealNetworks streams:
  unsigned fRealMaxBitRate, fRealAvgBitRate, fRealMaxPacketSize, fRealAvgPacketSize, fRealPreroll;
  char* fRealStreamName; char* fRealMIMEType;
  unsigned char* fRealOpaqueData; unsigned fRealOpaqueDataSize;
  // A pointer into "fRealOpaqueData":
  unsigned char* fRealTypeSpecificData; unsigned fRealTypeSpecificDataSize;
  unsigned fRealRuleNumber;
#endif

protected:
  friend class MediaSession;
  friend class MediaSubsessionIterator;
  MediaSubsession(MediaSession& parent);
  virtual ~MediaSubsession();

  UsageEnvironment& env() { return fParent.envir(); }
  void setNext(MediaSubsession* next) { fNext = next; }

  Boolean parseSDPLine_c(char const* sdpLine);
  Boolean parseSDPLine_b(char const* sdpLine);
  Boolean parseSDPAttribute_rtpmap(char const* sdpLine);
  Boolean parseSDPAttribute_control(char const* sdpLine);
  Boolean parseSDPAttribute_range(char const* sdpLine);
  Boolean parseSDPAttribute_fmtp(char const* sdpLine);
  Boolean parseSDPAttribute_source_filter(char const* sdpLine);
  Boolean parseSDPAttribute_x_dimensions(char const* sdpLine);
  Boolean parseSDPAttribute_framerate(char const* sdpLine);

protected:
  // Linkage fields:
  MediaSession& fParent;
  MediaSubsession* fNext;

  // Fields set from a SDP description:
  char* fConnectionEndpointName; // may also be set by RTSP SETUP response
  unsigned short fClientPortNum; // in host byte order
      // This field is also set by initiate()
  unsigned char fRTPPayloadFormat;
  char* fSavedSDPLines;
  char* fMediumName;
  char* fCodecName;
  char* fProtocolName;
  unsigned fRTPTimestampFrequency;
  char* fControlPath; // holds optional a=control: string
  struct in_addr fSourceFilterAddr; // used for SSM
  unsigned fBandwidth; // in kilobits-per-second, from b= line

  // Parameters set by "a=fmtp:" SDP lines:
  unsigned fAuxiliarydatasizelength, fConstantduration, fConstantsize;
  unsigned fCRC, fCtsdeltalength, fDe_interleavebuffersize, fDtsdeltalength;
  unsigned fIndexdeltalength, fIndexlength, fInterleaving;
  unsigned fMaxdisplacement, fObjecttype;
  unsigned fOctetalign, fProfile_level_id, fRobustsorting;
  unsigned fSizelength, fStreamstateindication, fStreamtype;
  Boolean fCpresent, fRandomaccessindication;
  char *fConfig, *fMode, *fSpropParameterSets;

  double fPlayStartTime;
  double fPlayEndTime;
  unsigned short fVideoWidth, fVideoHeight;
     // screen dimensions (set by an optional a=x-dimensions: <w>,<h> line)
  unsigned fVideoFPS;
     // frame rate (set by an optional "a=framerate: <fps>" or "a=x-framerate: <fps>" line)
  unsigned fNumChannels;
     // optionally set by "a=rtpmap:" lines for audio sessions.  Default: 1
  float fScale; // set from a RTSP "Scale:" header
  double fNPT_PTS_Offset; // set by "getNormalPlayTime()"; add this to a PTS to get NPT

  // Fields set by initiate():
  Groupsock* fRTPSocket; Groupsock* fRTCPSocket; // works even for unicast
  RTPSource* fRTPSource; RTCPInstance* fRTCPInstance;
  FramedSource* fReadSource;
};

#endif
