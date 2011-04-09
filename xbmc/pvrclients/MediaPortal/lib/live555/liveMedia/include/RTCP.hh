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
// RTCP
// C++ header

#ifndef _RTCP_HH
#define _RTCP_HH

#ifndef _RTP_SINK_HH
#include "RTPSink.hh"
#endif
#ifndef _RTP_SOURCE_HH
#include "RTPSource.hh"
#endif

class SDESItem {
public:
  SDESItem(unsigned char tag, unsigned char const* value);

  unsigned char const* data() const {return fData;}
  unsigned totalSize() const;

private:
  unsigned char fData[2 + 0xFF]; // first 2 bytes are tag and length
};

class RTCPMemberDatabase; // forward

class RTCPInstance: public Medium {
public:
  static RTCPInstance* createNew(UsageEnvironment& env, Groupsock* RTCPgs,
				 unsigned totSessionBW, /* in kbps */
				 unsigned char const* cname,
				 RTPSink* sink,
				 RTPSource const* source,
				 Boolean isSSMSource = False);

  static Boolean lookupByName(UsageEnvironment& env, char const* instanceName,
                              RTCPInstance*& resultInstance);

  unsigned numMembers() const;
  unsigned totSessionBW() const { return fTotSessionBW; }

  void setByeHandler(TaskFunc* handlerTask, void* clientData,
		     Boolean handleActiveParticipantsOnly = True);
      // Assigns a handler routine to be called if a "BYE" arrives.
      // The handler is called once only; for subsequent "BYE"s,
      // "setByeHandler()" would need to be called again.
      // If "handleActiveParticipantsOnly" is True, then the handler is called
      // only if the SSRC is for a known sender (if we have a "RTPSource"),
      // or if the SSRC is for a known receiver (if we have a "RTPSink").
      // This prevents (for example) the handler for a multicast receiver being
      // called if some other multicast receiver happens to exit.
      // If "handleActiveParticipantsOnly" is False, then the handler is called
      // for any incoming RTCP "BYE".
  void setSRHandler(TaskFunc* handlerTask, void* clientData);
  void setRRHandler(TaskFunc* handlerTask, void* clientData);
      // Assigns a handler routine to be called if a "SR" or "RR"
      // (respectively) arrives.  Unlike "setByeHandler()", the handler will
      // be called once for each incoming "SR" or "RR".  (To turn off handling,
      // call the function again with "handlerTask" (and "clientData") as NULL.
  void setSpecificRRHandler(netAddressBits fromAddress, Port fromPort,
			    TaskFunc* handlerTask, void* clientData);
      // Like "setRRHandler()", but applies only to "RR" packets that come from
      // a specific source address and port.  (Note that if both a specific
      // and a general "RR" handler function is set, then both will be called.)

  Groupsock* RTCPgs() const { return fRTCPInterface.gs(); }

  void setStreamSocket(int sockNum, unsigned char streamChannelId);
  void addStreamSocket(int sockNum, unsigned char streamChannelId);
  void removeStreamSocket(int sockNum, unsigned char streamChannelId) {
    fRTCPInterface.removeStreamSocket(sockNum, streamChannelId);
  }
    // hacks to allow sending RTP over TCP (RFC 2236, section 10.12)

  void setAuxilliaryReadHandler(AuxHandlerFunc* handlerFunc,
                                void* handlerClientData) {
    fRTCPInterface.setAuxilliaryReadHandler(handlerFunc,
					    handlerClientData);
  }

protected:
  RTCPInstance(UsageEnvironment& env, Groupsock* RTPgs, unsigned totSessionBW,
	       unsigned char const* cname,
	       RTPSink* sink, RTPSource const* source,
	       Boolean isSSMSource);
      // called only by createNew()
  virtual ~RTCPInstance();

private:
  // redefined virtual functions:
  virtual Boolean isRTCPInstance() const;

private:
  void addReport();
    void addSR();
    void addRR();
      void enqueueCommonReportPrefix(unsigned char packetType, u_int32_t SSRC,
				     unsigned numExtraWords = 0);
      void enqueueCommonReportSuffix();
        void enqueueReportBlock(RTPReceptionStats* receptionStats);
  void addSDES();
  void addBYE();

  void sendBuiltPacket();

  static void onExpire(RTCPInstance* instance);
  void onExpire1();

  static void incomingReportHandler(RTCPInstance* instance, int /*mask*/);
  void incomingReportHandler1();
  void onReceive(int typeOfPacket, int totPacketSize, u_int32_t ssrc);

  void unsetSpecificRRHandler(netAddressBits fromAddress, Port fromPort);

private:
  unsigned char* fInBuf;
  OutPacketBuffer* fOutBuf;
  RTPInterface fRTCPInterface;
  unsigned fTotSessionBW;
  RTPSink* fSink;
  RTPSource const* fSource;
  Boolean fIsSSMSource;

  SDESItem fCNAME;
  RTCPMemberDatabase* fKnownMembers;
  unsigned fOutgoingReportCount; // used for SSRC member aging

  double fAveRTCPSize;
  int fIsInitial;
  double fPrevReportTime;
  double fNextReportTime;
  int fPrevNumMembers;

  int fLastSentSize;
  int fLastReceivedSize;
  u_int32_t fLastReceivedSSRC;
  int fTypeOfEvent;
  int fTypeOfPacket;
  Boolean fHaveJustSentPacket;
  unsigned fLastPacketSentSize;

  TaskFunc* fByeHandlerTask;
  void* fByeHandlerClientData;
  Boolean fByeHandleActiveParticipantsOnly;
  TaskFunc* fSRHandlerTask;
  void* fSRHandlerClientData;
  TaskFunc* fRRHandlerTask;
  void* fRRHandlerClientData;
  AddressPortLookupTable* fSpecificRRHandlerTable;

public: // because this stuff is used by an external "C" function
  void schedule(double nextTime);
  void reschedule(double nextTime);
  void sendReport();
  void sendBYE();
  int typeOfEvent() {return fTypeOfEvent;}
  int sentPacketSize() {return fLastSentSize;}
  int packetType() {return fTypeOfPacket;}
  int receivedPacketSize() {return fLastReceivedSize;}
  int checkNewSSRC();
  void removeLastReceivedSSRC();
  void removeSSRC(u_int32_t ssrc, Boolean alsoRemoveStats);
};

// RTCP packet types:
const unsigned char RTCP_PT_SR = 200;
const unsigned char RTCP_PT_RR = 201;
const unsigned char RTCP_PT_SDES = 202;
const unsigned char RTCP_PT_BYE = 203;
const unsigned char RTCP_PT_APP = 204;

// SDES tags:
const unsigned char RTCP_SDES_END = 0;
const unsigned char RTCP_SDES_CNAME = 1;
const unsigned char RTCP_SDES_NAME = 2;
const unsigned char RTCP_SDES_EMAIL = 3;
const unsigned char RTCP_SDES_PHONE = 4;
const unsigned char RTCP_SDES_LOC = 5;
const unsigned char RTCP_SDES_TOOL = 6;
const unsigned char RTCP_SDES_NOTE = 7;
const unsigned char RTCP_SDES_PRIV = 8;

#endif
