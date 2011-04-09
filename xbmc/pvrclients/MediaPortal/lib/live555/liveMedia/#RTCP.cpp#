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
// Implementation

#include "RTCP.hh"
#include "GroupsockHelper.hh"
#include "rtcp_from_spec.h"

////////// RTCPMemberDatabase //////////

class RTCPMemberDatabase {
public:
  RTCPMemberDatabase(RTCPInstance& ourRTCPInstance)
    : fOurRTCPInstance(ourRTCPInstance), fNumMembers(1 /*ourself*/),
      fTable(HashTable::create(ONE_WORD_HASH_KEYS)) {
  }

  virtual ~RTCPMemberDatabase() {
	delete fTable;
  }

  Boolean isMember(unsigned ssrc) const {
    return fTable->Lookup((char*)(long)ssrc) != NULL;
  }

  Boolean noteMembership(unsigned ssrc, unsigned curTimeCount) {
    Boolean isNew = !isMember(ssrc);

    if (isNew) {
      ++fNumMembers;
    }

    // Record the current time, so we can age stale members
    fTable->Add((char*)(long)ssrc, (void*)(long)curTimeCount);

    return isNew;
  }

  Boolean remove(unsigned ssrc) {
    Boolean wasPresent = fTable->Remove((char*)(long)ssrc);
    if (wasPresent) {
      --fNumMembers;
    }
    return wasPresent;
  }

  unsigned numMembers() const {
    return fNumMembers;
  }

  void reapOldMembers(unsigned threshold);

private:
  RTCPInstance& fOurRTCPInstance;
  unsigned fNumMembers;
  HashTable* fTable;
};

void RTCPMemberDatabase::reapOldMembers(unsigned threshold) {
  Boolean foundOldMember;
  unsigned oldSSRC = 0;

  do {
    foundOldMember = False;

    HashTable::Iterator* iter
      = HashTable::Iterator::create(*fTable);
    unsigned long timeCount;
    char const* key;
    while ((timeCount = (unsigned long)(iter->next(key))) != 0) {
#ifdef DEBUG
      fprintf(stderr, "reap: checking SSRC 0x%lx: %ld (threshold %d)\n", (unsigned long)key, timeCount, threshold);
#endif
      if (timeCount < (unsigned long)threshold) { // this SSRC is old
        unsigned long ssrc = (unsigned long)key;
        oldSSRC = (unsigned)ssrc;
        foundOldMember = True;
      }
    }
    delete iter;

    if (foundOldMember) {
#ifdef DEBUG
        fprintf(stderr, "reap: removing SSRC 0x%x\n", oldSSRC);
#endif
      fOurRTCPInstance.removeSSRC(oldSSRC, True);
    }
  } while (foundOldMember);
}


////////// RTCPInstance //////////

static double dTimeNow() {
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    return (double) (timeNow.tv_sec + timeNow.tv_usec/1000000.0);
}

static unsigned const maxPacketSize = 1450;
	// bytes (1500, minus some allowance for IP, UDP, UMTP headers)
static unsigned const preferredPacketSize = 1000; // bytes

RTCPInstance::RTCPInstance(UsageEnvironment& env, Groupsock* RTCPgs,
			   unsigned totSessionBW,
			   unsigned char const* cname,
			   RTPSink* sink, RTPSource const* source,
			   Boolean isSSMSource)
  : Medium(env), fRTCPInterface(this, RTCPgs), fTotSessionBW(totSessionBW),
    fSink(sink), fSource(source), fIsSSMSource(isSSMSource),
    fCNAME(RTCP_SDES_CNAME, cname), fOutgoingReportCount(1),
    fAveRTCPSize(0), fIsInitial(1), fPrevNumMembers(0),
    fLastSentSize(0), fLastReceivedSize(0), fLastReceivedSSRC(0),
    fTypeOfEvent(EVENT_UNKNOWN), fTypeOfPacket(PACKET_UNKNOWN_TYPE),
    fHaveJustSentPacket(False), fLastPacketSentSize(0),
    fByeHandlerTask(NULL), fByeHandlerClientData(NULL),
    fSRHandlerTask(NULL), fSRHandlerClientData(NULL),
    fRRHandlerTask(NULL), fRRHandlerClientData(NULL),
    fSpecificRRHandlerTable(NULL) {
#ifdef DEBUG
  fprintf(stderr, "RTCPInstance[%p]::RTCPInstance()\n", this);
#endif
  if (fTotSessionBW == 0) { // not allowed!
    env << "RTCPInstance::RTCPInstance error: totSessionBW parameter should not be zero!\n";
    fTotSessionBW = 1;
  }

  if (isSSMSource) RTCPgs->multicastSendOnly(); // don't receive multicast

  double timeNow = dTimeNow();
  fPrevReportTime = fNextReportTime = timeNow;

  fKnownMembers = new RTCPMemberDatabase(*this);
  fInBuf = new unsigned char[maxPacketSize];
  if (fKnownMembers == NULL || fInBuf == NULL) return;

  // A hack to save buffer space, because RTCP packets are always small:
  unsigned savedMaxSize = OutPacketBuffer::maxSize;
  OutPacketBuffer::maxSize = maxPacketSize;
  fOutBuf = new OutPacketBuffer(preferredPacketSize, maxPacketSize);
  OutPacketBuffer::maxSize = savedMaxSize;
  if (fOutBuf == NULL) return;

  // Arrange to handle incoming reports from others:
  TaskScheduler::BackgroundHandlerProc* handler
    = (TaskScheduler::BackgroundHandlerProc*)&incomingReportHandler;
  fRTCPInterface.startNetworkReading(handler);

  // Send our first report.
  fTypeOfEvent = EVENT_REPORT;
  onExpire(this);
}

struct RRHandlerRecord {
  TaskFunc* rrHandlerTask;
  void* rrHandlerClientData;
};

RTCPInstance::~RTCPInstance() {
#ifdef DEBUG
  fprintf(stderr, "RTCPInstance[%p]::~RTCPInstance()\n", this);
#endif
  // Turn off background read handling:
  fRTCPInterface.stopNetworkReading();

  // Begin by sending a BYE.  We have to do this immediately, without
  // 'reconsideration', because "this" is going away.
  fTypeOfEvent = EVENT_BYE; // not used, but...
  sendBYE();

  if (fSpecificRRHandlerTable != NULL) {
    AddressPortLookupTable::Iterator iter(*fSpecificRRHandlerTable);
    RRHandlerRecord* rrHandler;
    while ((rrHandler = (RRHandlerRecord*)iter.next()) != NULL) {
      delete rrHandler;
    }
    delete fSpecificRRHandlerTable;
  }

  delete fKnownMembers;
  delete fOutBuf;
  delete[] fInBuf;
}

RTCPInstance* RTCPInstance::createNew(UsageEnvironment& env, Groupsock* RTCPgs,
				      unsigned totSessionBW,
				      unsigned char const* cname,
				      RTPSink* sink, RTPSource const* source,
				      Boolean isSSMSource) {
  return new RTCPInstance(env, RTCPgs, totSessionBW, cname, sink, source,
			  isSSMSource);
}

Boolean RTCPInstance::lookupByName(UsageEnvironment& env,
				   char const* instanceName,
				   RTCPInstance*& resultInstance) {
  resultInstance = NULL; // unless we succeed

  Medium* medium;
  if (!Medium::lookupByName(env, instanceName, medium)) return False;

  if (!medium->isRTCPInstance()) {
    env.setResultMsg(instanceName, " is not a RTCP instance");
    return False;
  }

  resultInstance = (RTCPInstance*)medium;
  return True;
}

Boolean RTCPInstance::isRTCPInstance() const {
  return True;
}

unsigned RTCPInstance::numMembers() const {
  if (fKnownMembers == NULL) return 0;

  return fKnownMembers->numMembers();
}

void RTCPInstance::setByeHandler(TaskFunc* handlerTask, void* clientData,
				 Boolean handleActiveParticipantsOnly) {
  fByeHandlerTask = handlerTask;
  fByeHandlerClientData = clientData;
  fByeHandleActiveParticipantsOnly = handleActiveParticipantsOnly;
}

void RTCPInstance::setSRHandler(TaskFunc* handlerTask, void* clientData) {
  fSRHandlerTask = handlerTask;
  fSRHandlerClientData = clientData;
}

void RTCPInstance::setRRHandler(TaskFunc* handlerTask, void* clientData) {
  fRRHandlerTask = handlerTask;
  fRRHandlerClientData = clientData;
}

void RTCPInstance
::setSpecificRRHandler(netAddressBits fromAddress, Port fromPort,
		       TaskFunc* handlerTask, void* clientData) {
  if (handlerTask == NULL && clientData == NULL) {
    unsetSpecificRRHandler(fromAddress, fromPort);
    return;
  }

  RRHandlerRecord* rrHandler = new RRHandlerRecord;
  rrHandler->rrHandlerTask = handlerTask;
  rrHandler->rrHandlerClientData = clientData;
  if (fSpecificRRHandlerTable == NULL) {
    fSpecificRRHandlerTable = new AddressPortLookupTable;
  }
  fSpecificRRHandlerTable->Add(fromAddress, (~0), fromPort, rrHandler);
}

void RTCPInstance
::unsetSpecificRRHandler(netAddressBits fromAddress, Port fromPort) {
  if (fSpecificRRHandlerTable == NULL) return;

  RRHandlerRecord* rrHandler
    = (RRHandlerRecord*)(fSpecificRRHandlerTable->Lookup(fromAddress, (~0), fromPort));
  if (rrHandler != NULL) {
    fSpecificRRHandlerTable->Remove(fromAddress, (~0), fromPort);
    delete rrHandler;
  }
}

void RTCPInstance::setStreamSocket(int sockNum,
				   unsigned char streamChannelId) {
  // Turn off background read handling:
  fRTCPInterface.stopNetworkReading();

  // Switch to RTCP-over-TCP:
  fRTCPInterface.setStreamSocket(sockNum, streamChannelId);

  // Turn background reading back on:
  TaskScheduler::BackgroundHandlerProc* handler
    = (TaskScheduler::BackgroundHandlerProc*)&incomingReportHandler;
  fRTCPInterface.startNetworkReading(handler);
}

void RTCPInstance::addStreamSocket(int sockNum,
				   unsigned char streamChannelId) {
  // First, turn off background read handling for the default (UDP) socket:
  fRTCPInterface.stopNetworkReading();

  // Add the RTCP-over-TCP interface:
  fRTCPInterface.setStreamSocket(sockNum, streamChannelId);

  // Turn on background reading for this socket (in case it's not on already):
  TaskScheduler::BackgroundHandlerProc* handler
    = (TaskScheduler::BackgroundHandlerProc*)&incomingReportHandler;
  fRTCPInterface.startNetworkReading(handler);
}

static unsigned const IP_UDP_HDR_SIZE = 28;
    // overhead (bytes) of IP and UDP hdrs

#define ADVANCE(n) pkt += (n); packetSize -= (n)

void RTCPInstance::incomingReportHandler(RTCPInstance* instance,
					 int /*mask*/) {
  instance->incomingReportHandler1();
}

void RTCPInstance::incomingReportHandler1() {
  unsigned char* pkt = fInBuf;
  unsigned packetSize;
  struct sockaddr_in fromAddress;
  int typeOfPacket = PACKET_UNKNOWN_TYPE;

  do {
    int tcpReadStreamSocketNum = fRTCPInterface.nextTCPReadStreamSocketNum();
    unsigned char tcpReadStreamChannelId = fRTCPInterface.nextTCPReadStreamChannelId();
    if (!fRTCPInterface.handleRead(pkt, maxPacketSize,
				   packetSize, fromAddress)) {
      break;
    }

    // Ignore the packet if it was looped-back from ourself:
    if (RTCPgs()->wasLoopedBackFromUs(envir(), fromAddress)) {
      // However, we still want to handle incoming RTCP packets from
      // *other processes* on the same machine.  To distinguish this
      // case from a true loop-back, check whether we've just sent a
      // packet of the same size.  (This check isn't perfect, but it seems
      // to be the best we can do.)
      if (fHaveJustSentPacket && fLastPacketSentSize == packetSize) {
	// This is a true loop-back:
	fHaveJustSentPacket = False;
	break; // ignore this packet
      }
    }

    if (fIsSSMSource) {
      // This packet was received via unicast.  'Reflect' it by resending
      // it to the multicast group.
      // NOTE: Denial-of-service attacks are possible here.
      // Users of this software may wish to add their own,
      // application-specific mechanism for 'authenticating' the
      // validity of this packet before reflecting it.
      fRTCPInterface.sendPacket(pkt, packetSize);
      fHaveJustSentPacket = True;
      fLastPacketSentSize = packetSize;
    }

#ifdef DEBUG
    fprintf(stderr, "[%p]saw incoming RTCP packet (from address %s, port %d)\n", this, our_inet_ntoa(fromAddress.sin_addr), ntohs(fromAddress.sin_port));
    unsigned char* p = pkt;
    for (unsigned i = 0; i < packetSize; ++i) {
      if (i%4 == 0) fprintf(stderr, " ");
      fprintf(stderr, "%02x", p[i]);
    }
    fprintf(stderr, "\n");
#endif
    int totPacketSize = IP_UDP_HDR_SIZE + packetSize;

    // Check the RTCP packet for validity:
    // It must at least contain a header (4 bytes), and this header
    // must be version=2, with no padding bit, and a payload type of
    // SR (200) or RR (201):
    if (packetSize < 4) break;
    unsigned rtcpHdr = ntohl(*(unsigned*)pkt);
    if ((rtcpHdr & 0xE0FE0000) != (0x80000000 | (RTCP_PT_SR<<16))) {
#ifdef DEBUG
      fprintf(stderr, "rejected bad RTCP packet: header 0x%08x\n", rtcpHdr);
#endif
      break;
    }

    // Process each of the individual RTCP 'subpackets' in (what may be)
    // a compound RTCP packet.
    unsigned reportSenderSSRC = 0;
    Boolean packetOK = False;
    while (1) {
      unsigned rc = (rtcpHdr>>24)&0x1F;
      unsigned pt = (rtcpHdr>>16)&0xFF;
      unsigned length = 4*(rtcpHdr&0xFFFF); // doesn't count hdr
      ADVANCE(4); // skip over the header
      if (length > packetSize) break;

      // Assume that each RTCP subpacket begins with a 4-byte SSRC:
      if (length < 4) break; length -= 4;
      reportSenderSSRC = ntohl(*(unsigned*)pkt); ADVANCE(4);

      Boolean subPacketOK = False;
      switch (pt) {
        case RTCP_PT_SR: {
#ifdef DEBUG
	  fprintf(stderr, "SR\n");
#endif
	  if (length < 20) break; length -= 20;

	  // Extract the NTP timestamp, and note this:
	  unsigned NTPmsw = ntohl(*(unsigned*)pkt); ADVANCE(4);
	  unsigned NTPlsw = ntohl(*(unsigned*)pkt); ADVANCE(4);
	  unsigned rtpTimestamp = ntohl(*(unsigned*)pkt); ADVANCE(4);
	  if (fSource != NULL) {
	    RTPReceptionStatsDB& receptionStats
	      = fSource->receptionStatsDB();
	    receptionStats.noteIncomingSR(reportSenderSSRC,
					  NTPmsw, NTPlsw, rtpTimestamp);
	  }
	  ADVANCE(8); // skip over packet count, octet count

	  // If a 'SR handler' was set, call it now:
	  if (fSRHandlerTask != NULL) (*fSRHandlerTask)(fSRHandlerClientData);

	  // The rest of the SR is handled like a RR (so, no "break;" here)
	}
        case RTCP_PT_RR: {
#ifdef DEBUG
	  fprintf(stderr, "RR\n");
#endif
	  unsigned reportBlocksSize = rc*(6*4);
	  if (length < reportBlocksSize) break;
	  length -= reportBlocksSize;

          if (fSink != NULL) {
	    // Use this information to update stats about our transmissions:
            RTPTransmissionStatsDB& transmissionStats = fSink->transmissionStatsDB();
            for (unsigned i = 0; i < rc; ++i) {
              unsigned senderSSRC = ntohl(*(unsigned*)pkt); ADVANCE(4);
              // We care only about reports about our own transmission, not others'
              if (senderSSRC == fSink->SSRC()) {
                unsigned lossStats = ntohl(*(unsigned*)pkt); ADVANCE(4);
                unsigned highestReceived = ntohl(*(unsigned*)pkt); ADVANCE(4);
                unsigned jitter = ntohl(*(unsigned*)pkt); ADVANCE(4);
                unsigned timeLastSR = ntohl(*(unsigned*)pkt); ADVANCE(4);
                unsigned timeSinceLastSR = ntohl(*(unsigned*)pkt); ADVANCE(4);
                transmissionStats.noteIncomingRR(reportSenderSSRC, fromAddress,
						 lossStats,
						 highestReceived, jitter,
						 timeLastSR, timeSinceLastSR);
              } else {
                ADVANCE(4*5);
              }
            }
          } else {
            ADVANCE(reportBlocksSize);
          }

	  if (pt == RTCP_PT_RR) { // i.e., we didn't fall through from 'SR'
	    // If a 'RR handler' was set, call it now:

	    // Specific RR handler:
	    if (fSpecificRRHandlerTable != NULL) {
	      netAddressBits fromAddr;
	      portNumBits fromPortNum;
	      if (tcpReadStreamSocketNum < 0) {
		// Normal case: We read the RTCP packet over UDP
		fromAddr = fromAddress.sin_addr.s_addr;
		fromPortNum = ntohs(fromAddress.sin_port);
	      } else {
		// Special case: We read the RTCP packet over TCP (interleaved)
		// Hack: Use the TCP socket and channel id to look up the handler
		fromAddr = tcpReadStreamSocketNum;
		fromPortNum = tcpReadStreamChannelId;
	      }
	      Port fromPort(fromPortNum);
	      RRHandlerRecord* rrHandler
		= (RRHandlerRecord*)(fSpecificRRHandlerTable->Lookup(fromAddr, (~0), fromPort));
	      if (rrHandler != NULL) {
		if (rrHandler->rrHandlerTask != NULL) {
		  (*(rrHandler->rrHandlerTask))(rrHandler->rrHandlerClientData);
		}
	      }
	    }

	    // General RR handler:
	    if (fRRHandlerTask != NULL) (*fRRHandlerTask)(fRRHandlerClientData);
	  }

	  subPacketOK = True;
	  typeOfPacket = PACKET_RTCP_REPORT;
	  break;
	}
        case RTCP_PT_BYE: {
#ifdef DEBUG
	  fprintf(stderr, "BYE\n");
#endif
	  // If a 'BYE handler' was set, call it now:
	  TaskFunc* byeHandler = fByeHandlerTask;
	  if (byeHandler != NULL
	      && (!fByeHandleActiveParticipantsOnly
		  || (fSource != NULL
		      && fSource->receptionStatsDB().lookup(reportSenderSSRC) != NULL)
		  || (fSink != NULL
		      && fSink->transmissionStatsDB().lookup(reportSenderSSRC) != NULL))) {
	    fByeHandlerTask = NULL;
	        // we call this only once by default
	    (*byeHandler)(fByeHandlerClientData);
	  }

	  // We should really check for & handle >1 SSRCs being present #####

	  subPacketOK = True;
	  typeOfPacket = PACKET_BYE;
	  break;
	}
	// Later handle SDES, APP, and compound RTCP packets #####
        default:
#ifdef DEBUG
	  fprintf(stderr, "UNSUPPORTED TYPE(0x%x)\n", pt);
#endif
	  subPacketOK = True;
	  break;
      }
      if (!subPacketOK) break;

      // need to check for (& handle) SSRC collision! #####

#ifdef DEBUG
      fprintf(stderr, "validated RTCP subpacket (type %d): %d, %d, %d, 0x%08x\n", typeOfPacket, rc, pt, length, reportSenderSSRC);
#endif

      // Skip over any remaining bytes in this subpacket:
      ADVANCE(length);

      // Check whether another RTCP 'subpacket' follows:
      if (packetSize == 0) {
	packetOK = True;
	break;
      } else if (packetSize < 4) {
#ifdef DEBUG
	fprintf(stderr, "extraneous %d bytes at end of RTCP packet!\n", packetSize);
#endif
	break;
      }
      rtcpHdr = ntohl(*(unsigned*)pkt);
      if ((rtcpHdr & 0xC0000000) != 0x80000000) {
#ifdef DEBUG
	fprintf(stderr, "bad RTCP subpacket: header 0x%08x\n", rtcpHdr);
#endif
	break;
      }
    }

    if (!packetOK) {
#ifdef DEBUG
      fprintf(stderr, "rejected bad RTCP subpacket: header 0x%08x\n", rtcpHdr);
#endif
      break;
    } else {
#ifdef DEBUG
      fprintf(stderr, "validated entire RTCP packet\n");
#endif
    }

    onReceive(typeOfPacket, totPacketSize, reportSenderSSRC);
  } while (0);
}

void RTCPInstance::onReceive(int typeOfPacket, int totPacketSize,
			     unsigned ssrc) {
  fTypeOfPacket = typeOfPacket;
  fLastReceivedSize = totPacketSize;
  fLastReceivedSSRC = ssrc;

  int members = (int)numMembers();
  int senders = (fSink != NULL) ? 1 : 0;

  OnReceive(this, // p
	    this, // e
	    &members, // members
	    &fPrevNumMembers, // pmembers
	    &senders, // senders
	    &fAveRTCPSize, // avg_rtcp_size
	    &fPrevReportTime, // tp
	    dTimeNow(), // tc
	    fNextReportTime);
}

void RTCPInstance::sendReport() {
  // Hack: Don't send a SR during those (brief) times when the timestamp of the
  // next outgoing RTP packet has been preset, to ensure that that timestamp gets
  // used for that outgoing packet. (David Bertrand, 2006.07.18)
  if (fSink != NULL && fSink->nextTimestampHasBeenPreset()) return;

#ifdef DEBUG
  fprintf(stderr, "sending REPORT\n");
#endif
  // Begin by including a SR and/or RR report:
  addReport();

  // Then, include a SDES:
  addSDES();

  // Send the report:
  sendBuiltPacket();

  // Periodically clean out old members from our SSRC membership database:
  const unsigned membershipReapPeriod = 5;
  if ((++fOutgoingReportCount) % membershipReapPeriod == 0) {
    unsigned threshold = fOutgoingReportCount - membershipReapPeriod;
    fKnownMembers->reapOldMembers(threshold);
  }
}

void RTCPInstance::sendBYE() {
#ifdef DEBUG
  fprintf(stderr, "sending BYE\n");
#endif
  // The packet must begin with a SR and/or RR report:
  addReport();

  addBYE();
  sendBuiltPacket();
}

void RTCPInstance::sendBuiltPacket() {
#ifdef DEBUG
  fprintf(stderr, "sending RTCP packet\n");
  unsigned char* p = fOutBuf->packet();
  for (unsigned i = 0; i < fOutBuf->curPacketSize(); ++i) {
    if (i%4 == 0) fprintf(stderr," ");
    fprintf(stderr, "%02x", p[i]);
  }
  fprintf(stderr, "\n");
#endif
  unsigned reportSize = fOutBuf->curPacketSize();
  fRTCPInterface.sendPacket(fOutBuf->packet(), reportSize);
  fOutBuf->resetOffset();

  fLastSentSize = IP_UDP_HDR_SIZE + reportSize;
  fHaveJustSentPacket = True;
  fLastPacketSentSize = reportSize;
}

int RTCPInstance::checkNewSSRC() {
  return fKnownMembers->noteMembership(fLastReceivedSSRC,
				       fOutgoingReportCount);
}

void RTCPInstance::removeLastReceivedSSRC() {
  removeSSRC(fLastReceivedSSRC, False/*keep stats around*/);
}

void RTCPInstance::removeSSRC(u_int32_t ssrc, Boolean alsoRemoveStats) {
  fKnownMembers->remove(ssrc);

  if (alsoRemoveStats) {
    // Also, remove records of this SSRC from any reception or transmission stats
    if (fSource != NULL) fSource->receptionStatsDB().removeRecord(ssrc);
    if (fSink != NULL) fSink->transmissionStatsDB().removeRecord(ssrc);
  }
}

void RTCPInstance::onExpire(RTCPInstance* instance) {
  instance->onExpire1();
}

// Member functions to build specific kinds of report:

void RTCPInstance::addReport() {
  // Include a SR or a RR, depending on whether we
  // have an associated sink or source:
  if (fSink != NULL) {
    addSR();
  } else if (fSource != NULL) {
    addRR();
  }
}

void RTCPInstance::addSR() {
  // ASSERT: fSink != NULL

  enqueueCommonReportPrefix(RTCP_PT_SR, fSink->SSRC(),
			    5 /* extra words in a SR */);

  // Now, add the 'sender info' for our sink

  // Insert the NTP and RTP timestamps for the 'wallclock time':
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  fOutBuf->enqueueWord(timeNow.tv_sec + 0x83AA7E80);
      // NTP timestamp most-significant word (1970 epoch -> 1900 epoch)
  double fractionalPart = (timeNow.tv_usec/15625.0)*0x04000000; // 2^32/10^6
  fOutBuf->enqueueWord((unsigned)(fractionalPart+0.5));
      // NTP timestamp least-significant word
  unsigned rtpTimestamp = fSink->convertToRTPTimestamp(timeNow);
  fOutBuf->enqueueWord(rtpTimestamp); // RTP ts

  // Insert the packet and byte counts:
  fOutBuf->enqueueWord(fSink->packetCount());
  fOutBuf->enqueueWord(fSink->octetCount());

  enqueueCommonReportSuffix();
}

void RTCPInstance::addRR() {
  // ASSERT: fSource != NULL

  enqueueCommonReportPrefix(RTCP_PT_RR, fSource->SSRC());
  enqueueCommonReportSuffix();
}

void RTCPInstance::enqueueCommonReportPrefix(unsigned char packetType,
					     unsigned SSRC,
					     unsigned numExtraWords) {
  unsigned numReportingSources;
  if (fSource == NULL) {
    numReportingSources = 0; // we don't receive anything
  } else {
    RTPReceptionStatsDB& allReceptionStats
      = fSource->receptionStatsDB();
    numReportingSources = allReceptionStats.numActiveSourcesSinceLastReset();
    // This must be <32, to fit in 5 bits:
    if (numReportingSources >= 32) { numReportingSources = 32; }
    // Later: support adding more reports to handle >32 sources (unlikely)#####
  }

  unsigned rtcpHdr = 0x80000000; // version 2, no padding
  rtcpHdr |= (numReportingSources<<24);
  rtcpHdr |= (packetType<<16);
  rtcpHdr |= (1 + numExtraWords + 6*numReportingSources);
      // each report block is 6 32-bit words long
  fOutBuf->enqueueWord(rtcpHdr);

  fOutBuf->enqueueWord(SSRC);
}

void RTCPInstance::enqueueCommonReportSuffix() {
  // Output the report blocks for each source:
  if (fSource != NULL) {
    RTPReceptionStatsDB& allReceptionStats
      = fSource->receptionStatsDB();

    RTPReceptionStatsDB::Iterator iterator(allReceptionStats);
    while (1) {
      RTPReceptionStats* receptionStats = iterator.next();
      if (receptionStats == NULL) break;
      enqueueReportBlock(receptionStats);
    }

    allReceptionStats.reset(); // because we have just generated a report
  }
}

void
RTCPInstance::enqueueReportBlock(RTPReceptionStats* stats) {
  fOutBuf->enqueueWord(stats->SSRC());

  unsigned highestExtSeqNumReceived = stats->highestExtSeqNumReceived();

  unsigned totNumExpected
    = highestExtSeqNumReceived - stats->baseExtSeqNumReceived();
  int totNumLost = totNumExpected - stats->totNumPacketsReceived();
  // 'Clamp' this loss number to a 24-bit signed value:
  if (totNumLost > 0x007FFFFF) {
    totNumLost = 0x007FFFFF;
  } else if (totNumLost < 0) {
    if (totNumLost < -0x00800000) totNumLost = 0x00800000; // unlikely, but...
    totNumLost &= 0x00FFFFFF;
  }

  unsigned numExpectedSinceLastReset
    = highestExtSeqNumReceived - stats->lastResetExtSeqNumReceived();
  int numLostSinceLastReset
    = numExpectedSinceLastReset - stats->numPacketsReceivedSinceLastReset();
  unsigned char lossFraction;
  if (numExpectedSinceLastReset == 0 || numLostSinceLastReset < 0) {
    lossFraction = 0;
  } else {
    lossFraction = (unsigned char)
      ((numLostSinceLastReset << 8) / numExpectedSinceLastReset);
  }

  fOutBuf->enqueueWord((lossFraction<<24) | totNumLost);
  fOutBuf->enqueueWord(highestExtSeqNumReceived);

  fOutBuf->enqueueWord(stats->jitter());

  unsigned NTPmsw = stats->lastReceivedSR_NTPmsw();
  unsigned NTPlsw = stats->lastReceivedSR_NTPlsw();
  unsigned LSR = ((NTPmsw&0xFFFF)<<16)|(NTPlsw>>16); // middle 32 bits
  fOutBuf->enqueueWord(LSR);

  // Figure out how long has elapsed since the last SR rcvd from this src:
  struct timeval const& LSRtime = stats->lastReceivedSR_time(); // "last SR"
  struct timeval timeNow, timeSinceLSR;
  gettimeofday(&timeNow, NULL);
  if (timeNow.tv_usec < LSRtime.tv_usec) {
    timeNow.tv_usec += 1000000;
    timeNow.tv_sec -= 1;
  }
  timeSinceLSR.tv_sec = timeNow.tv_sec - LSRtime.tv_sec;
  timeSinceLSR.tv_usec = timeNow.tv_usec - LSRtime.tv_usec;
  // The enqueued time is in units of 1/65536 seconds.
  // (Note that 65536/1000000 == 1024/15625)
  unsigned DLSR;
  if (LSR == 0) {
    DLSR = 0;
  } else {
    DLSR = (timeSinceLSR.tv_sec<<16)
         | ( (((timeSinceLSR.tv_usec<<11)+15625)/31250) & 0xFFFF);
  }
  fOutBuf->enqueueWord(DLSR);
}

void RTCPInstance::addSDES() {
  // For now we support only the CNAME item; later support more #####

  // Begin by figuring out the size of the entire SDES report:
  unsigned numBytes = 4;
      // counts the SSRC, but not the header; it'll get subtracted out
  numBytes += fCNAME.totalSize(); // includes id and length
  numBytes += 1; // the special END item

  unsigned num4ByteWords = (numBytes + 3)/4;

  unsigned rtcpHdr = 0x81000000; // version 2, no padding, 1 SSRC chunk
  rtcpHdr |= (RTCP_PT_SDES<<16);
  rtcpHdr |= num4ByteWords;
  fOutBuf->enqueueWord(rtcpHdr);

  if (fSource != NULL) {
    fOutBuf->enqueueWord(fSource->SSRC());
  } else if (fSink != NULL) {
    fOutBuf->enqueueWord(fSink->SSRC());
  }

  // Add the CNAME:
  fOutBuf->enqueue(fCNAME.data(), fCNAME.totalSize());

  // Add the 'END' item (i.e., a zero byte), plus any more needed to pad:
  unsigned numPaddingBytesNeeded = 4 - (fOutBuf->curPacketSize() % 4);
  unsigned char const zero = '\0';
  while (numPaddingBytesNeeded-- > 0) fOutBuf->enqueue(&zero, 1);
}

void RTCPInstance::addBYE() {
  unsigned rtcpHdr = 0x81000000; // version 2, no padding, 1 SSRC
  rtcpHdr |= (RTCP_PT_BYE<<16);
  rtcpHdr |= 1; // 2 32-bit words total (i.e., with 1 SSRC)
  fOutBuf->enqueueWord(rtcpHdr);

  if (fSource != NULL) {
    fOutBuf->enqueueWord(fSource->SSRC());
  } else if (fSink != NULL) {
    fOutBuf->enqueueWord(fSink->SSRC());
  }
}

void RTCPInstance::schedule(double nextTime) {
  fNextReportTime = nextTime;

  double secondsToDelay = nextTime - dTimeNow();
#ifdef DEBUG
  fprintf(stderr, "schedule(%f->%f)\n", secondsToDelay, nextTime);
#endif
  int usToGo = (int)(secondsToDelay * 1000000);
  nextTask() = envir().taskScheduler().scheduleDelayedTask(usToGo,
				(TaskFunc*)RTCPInstance::onExpire, this);
}

void RTCPInstance::reschedule(double nextTime) {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  schedule(nextTime);
}

void RTCPInstance::onExpire1() {
  // Note: fTotSessionBW is kbits per second
  double rtcpBW = 0.05*fTotSessionBW*1024/8; // -> bytes per second

  OnExpire(this, // event
	   numMembers(), // members
	   (fSink != NULL) ? 1 : 0, // senders
	   rtcpBW, // rtcp_bw
	   (fSink != NULL) ? 1 : 0, // we_sent
	   &fAveRTCPSize, // ave_rtcp_size
	   &fIsInitial, // initial
	   dTimeNow(), // tc
	   &fPrevReportTime, // tp
	   &fPrevNumMembers // pmembers
	   );
}

////////// SDESItem //////////

SDESItem::SDESItem(unsigned char tag, unsigned char const* value) {
  unsigned length = strlen((char const*)value);
  if (length > 511) length = 511;

  fData[0] = tag;
  fData[1] = (unsigned char)length;
  memmove(&fData[2], value, length);

  // Pad the trailing bytes to a 4-byte boundary:
  while ((length)%4 > 0) fData[2 + length++] = '\0';
}

unsigned SDESItem::totalSize() const {
  return 2 + (unsigned)fData[1];
}


////////// Implementation of routines imported by the "rtcp_from_spec" C code

extern "C" void Schedule(double nextTime, event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return;

  instance->schedule(nextTime);
}

extern "C" void Reschedule(double nextTime, event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return;

  instance->reschedule(nextTime);
}

extern "C" void SendRTCPReport(event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return;

  instance->sendReport();
}

extern "C" void SendBYEPacket(event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return;

  instance->sendBYE();
}

extern "C" int TypeOfEvent(event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return EVENT_UNKNOWN;

  return instance->typeOfEvent();
}

extern "C" int SentPacketSize(event e) {
  RTCPInstance* instance = (RTCPInstance*)e;
  if (instance == NULL) return 0;

  return instance->sentPacketSize();
}

extern "C" int PacketType(packet p) {
  RTCPInstance* instance = (RTCPInstance*)p;
  if (instance == NULL) return PACKET_UNKNOWN_TYPE;

  return instance->packetType();
}

extern "C" int ReceivedPacketSize(packet p) {
  RTCPInstance* instance = (RTCPInstance*)p;
  if (instance == NULL) return 0;

  return instance->receivedPacketSize();
}

extern "C" int NewMember(packet p) {
  RTCPInstance* instance = (RTCPInstance*)p;
  if (instance == NULL) return 0;

  return instance->checkNewSSRC();
}

extern "C" int NewSender(packet /*p*/) {
  return 0; // we don't yet recognize senders other than ourselves #####
}

extern "C" void AddMember(packet /*p*/) {
  // Do nothing; all of the real work was done when NewMember() was called
}

extern "C" void AddSender(packet /*p*/) {
  // we don't yet recognize senders other than ourselves #####
}

extern "C" void RemoveMember(packet p) {
  RTCPInstance* instance = (RTCPInstance*)p;
  if (instance == NULL) return;

  instance->removeLastReceivedSSRC();
}

extern "C" void RemoveSender(packet /*p*/) {
  // we don't yet recognize senders other than ourselves #####
}

extern "C" double drand30() {
  unsigned tmp = our_random()&0x3FFFFFFF; // a random 30-bit integer
  return tmp/(double)(1024*1024*1024);
}
