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
// RTP Sinks
// Implementation

#include "RTPSink.hh"
#include "GroupsockHelper.hh"

////////// RTPSink //////////

Boolean RTPSink::lookupByName(UsageEnvironment& env, char const* sinkName,
				RTPSink*& resultSink) {
  resultSink = NULL; // unless we succeed

  MediaSink* sink;
  if (!MediaSink::lookupByName(env, sinkName, sink)) return False;

  if (!sink->isRTPSink()) {
    env.setResultMsg(sinkName, " is not a RTP sink");
    return False;
  }

  resultSink = (RTPSink*)sink;
  return True;
}

Boolean RTPSink::isRTPSink() const {
  return True;
}

RTPSink::RTPSink(UsageEnvironment& env,
		 Groupsock* rtpGS, unsigned char rtpPayloadType,
		 unsigned rtpTimestampFrequency,
		 char const* rtpPayloadFormatName,
		 unsigned numChannels)
  : MediaSink(env), fRTPInterface(this, rtpGS),
    fRTPPayloadType(rtpPayloadType),
    fPacketCount(0), fOctetCount(0), fTotalOctetCount(0),
    fTimestampFrequency(rtpTimestampFrequency), fNextTimestampHasBeenPreset(True),
    fNumChannels(numChannels) {
  fRTPPayloadFormatName
    = strDup(rtpPayloadFormatName == NULL ? "???" : rtpPayloadFormatName);
  gettimeofday(&fCreationTime, NULL);
  fTotalOctetCountStartTime = fCreationTime;

  fSeqNo = (u_int16_t)our_random();
  fSSRC = our_random32();
  fTimestampBase = our_random32();

  fTransmissionStatsDB = new RTPTransmissionStatsDB(*this);
}

RTPSink::~RTPSink() {
  delete fTransmissionStatsDB;
  delete[] (char*)fRTPPayloadFormatName;
}

u_int32_t RTPSink::convertToRTPTimestamp(struct timeval tv) {
  // Begin by converting from "struct timeval" units to RTP timestamp units:
  u_int32_t timestampIncrement = (fTimestampFrequency*tv.tv_sec);
  timestampIncrement += (u_int32_t)((2.0*fTimestampFrequency*tv.tv_usec + 1000000.0)/2000000);
       // note: rounding

  // Then add this to our 'timestamp base':
  if (fNextTimestampHasBeenPreset) {
    // Make the returned timestamp the same as the current "fTimestampBase",
    // so that timestamps begin with the value that was previously preset:
    fTimestampBase -= timestampIncrement;
    fNextTimestampHasBeenPreset = False;
  }

  u_int32_t const rtpTimestamp = fTimestampBase + timestampIncrement;
#ifdef DEBUG_TIMESTAMPS
  fprintf(stderr, "fTimestampBase: 0x%08x, tv: %lu.%06ld\n\t=> RTP timestamp: 0x%08x\n",
	  fTimestampBase, tv.tv_sec, tv.tv_usec, rtpTimestamp);
  fflush(stderr);
#endif

  return rtpTimestamp;
}

u_int32_t RTPSink::presetNextTimestamp() {
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);

  u_int32_t tsNow = convertToRTPTimestamp(timeNow);
  fTimestampBase = tsNow;
  fNextTimestampHasBeenPreset = True;

  return tsNow;
}

void RTPSink::getTotalBitrate(unsigned& outNumBytes, double& outElapsedTime) {
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);

  outNumBytes = fTotalOctetCount;
  outElapsedTime = (double)(timeNow.tv_sec-fTotalOctetCountStartTime.tv_sec)
    + (timeNow.tv_usec-fTotalOctetCountStartTime.tv_usec)/1000000.0;

  fTotalOctetCount = 0;
  fTotalOctetCountStartTime = timeNow;
}

char const* RTPSink::sdpMediaType() const {
  return "data";
  // default SDP media (m=) type, unless redefined by subclasses
}

char* RTPSink::rtpmapLine() const {
  if (rtpPayloadType() >= 96) { // the payload format type is dynamic
    char* encodingParamsPart;
    if (numChannels() != 1) {
      encodingParamsPart = new char[1 + 20 /* max int len */];
      sprintf(encodingParamsPart, "/%d", numChannels());
    } else {
      encodingParamsPart = strDup("");
    }
    char const* const rtpmapFmt = "a=rtpmap:%d %s/%d%s\r\n";
    unsigned rtpmapFmtSize = strlen(rtpmapFmt)
      + 3 /* max char len */ + strlen(rtpPayloadFormatName())
      + 20 /* max int len */ + strlen(encodingParamsPart);
    char* rtpmapLine = new char[rtpmapFmtSize];
    sprintf(rtpmapLine, rtpmapFmt,
	    rtpPayloadType(), rtpPayloadFormatName(),
	    rtpTimestampFrequency(), encodingParamsPart);
    delete[] encodingParamsPart;

    return rtpmapLine;
  } else {
    // The payload format is staic, so there's no "a=rtpmap:" line:
    return strDup("");
  }
}

char const* RTPSink::auxSDPLine() {
  return NULL; // by default
}


////////// RTPTransmissionStatsDB //////////

RTPTransmissionStatsDB::RTPTransmissionStatsDB(RTPSink& rtpSink)
  : fOurRTPSink(rtpSink),
    fTable(HashTable::create(ONE_WORD_HASH_KEYS)) {
  fNumReceivers=0;
}

RTPTransmissionStatsDB::~RTPTransmissionStatsDB() {
  // First, remove and delete all stats records from the table:
  RTPTransmissionStats* stats;
  while ((stats = (RTPTransmissionStats*)fTable->RemoveNext()) != NULL) {
    delete stats;
  }

  // Then, delete the table itself:
  delete fTable;
}

void RTPTransmissionStatsDB
::noteIncomingRR(u_int32_t SSRC, struct sockaddr_in const& lastFromAddress,
                 unsigned lossStats, unsigned lastPacketNumReceived,
                 unsigned jitter, unsigned lastSRTime, unsigned diffSR_RRTime) {
  RTPTransmissionStats* stats = lookup(SSRC);
  if (stats == NULL) {
    // This is the first time we've heard of this SSRC.
    // Create a new record for it:
    stats = new RTPTransmissionStats(fOurRTPSink, SSRC);
    if (stats == NULL) return;
    add(SSRC, stats);
#ifdef DEBUG_RR
    fprintf(stderr, "Adding new entry for SSRC %x in RTPTransmissionStatsDB\n", SSRC);
#endif
  }

  stats->noteIncomingRR(lastFromAddress,
			lossStats, lastPacketNumReceived, jitter,
                        lastSRTime, diffSR_RRTime);
}

void RTPTransmissionStatsDB::removeRecord(u_int32_t SSRC) {
  RTPTransmissionStats* stats = lookup(SSRC);
  if (stats != NULL) {
    long SSRC_long = (long)SSRC;
    fTable->Remove((char const*)SSRC_long);
    --fNumReceivers;
    delete stats;
  }
}

RTPTransmissionStatsDB::Iterator
::Iterator(RTPTransmissionStatsDB& receptionStatsDB)
  : fIter(HashTable::Iterator::create(*(receptionStatsDB.fTable))) {
}

RTPTransmissionStatsDB::Iterator::~Iterator() {
  delete fIter;
}

RTPTransmissionStats*
RTPTransmissionStatsDB::Iterator::next() {
  char const* key; // dummy

  return (RTPTransmissionStats*)(fIter->next(key));
}

RTPTransmissionStats* RTPTransmissionStatsDB::lookup(u_int32_t SSRC) const {
  long SSRC_long = (long)SSRC;
  return (RTPTransmissionStats*)(fTable->Lookup((char const*)SSRC_long));
}

void RTPTransmissionStatsDB::add(u_int32_t SSRC, RTPTransmissionStats* stats) {
  long SSRC_long = (long)SSRC;
  fTable->Add((char const*)SSRC_long, stats);
  ++fNumReceivers;
}


////////// RTPTransmissionStats //////////

RTPTransmissionStats::RTPTransmissionStats(RTPSink& rtpSink, u_int32_t SSRC)
  : fOurRTPSink(rtpSink), fSSRC(SSRC), fLastPacketNumReceived(0),
    fPacketLossRatio(0), fTotNumPacketsLost(0), fJitter(0),
    fLastSRTime(0), fDiffSR_RRTime(0), fFirstPacket(True),
    fTotalOctetCount_hi(0), fTotalOctetCount_lo(0),
    fTotalPacketCount_hi(0), fTotalPacketCount_lo(0) {
  gettimeofday(&fTimeCreated, NULL);

  fLastOctetCount = rtpSink.octetCount();
  fLastPacketCount = rtpSink.packetCount();
}

RTPTransmissionStats::~RTPTransmissionStats() {}

void RTPTransmissionStats
::noteIncomingRR(struct sockaddr_in const& lastFromAddress,
		 unsigned lossStats, unsigned lastPacketNumReceived,
		 unsigned jitter, unsigned lastSRTime,
		 unsigned diffSR_RRTime) {
  if (fFirstPacket) {
    fFirstPacket = False;
    fFirstPacketNumReported = lastPacketNumReceived;
  } else {
    fOldValid = True;
    fOldLastPacketNumReceived = fLastPacketNumReceived;
    fOldTotNumPacketsLost = fTotNumPacketsLost;
  }
  gettimeofday(&fTimeReceived, NULL);

  fLastFromAddress = lastFromAddress;
  fPacketLossRatio = lossStats>>24;
  fTotNumPacketsLost = lossStats&0xFFFFFF;
  fLastPacketNumReceived = lastPacketNumReceived;
  fJitter = jitter;
  fLastSRTime = lastSRTime;
  fDiffSR_RRTime = diffSR_RRTime;
#ifdef DEBUG_RR
  fprintf(stderr, "RTCP RR data (received at %lu.%06ld): lossStats 0x%08x, lastPacketNumReceived 0x%08x, jitter 0x%08x, lastSRTime 0x%08x, diffSR_RRTime 0x%08x\n",
          fTimeReceived.tv_sec, fTimeReceived.tv_usec, lossStats, lastPacketNumReceived, jitter, lastSRTime, diffSR_RRTime);
  unsigned rtd = roundTripDelay();
  fprintf(stderr, "=> round-trip delay: 0x%04x (== %f seconds)\n", rtd, rtd/65536.0);
#endif

  // Update our counts of the total number of octets and packets sent towards
  // this receiver:
  u_int32_t newOctetCount = fOurRTPSink.octetCount();
  u_int32_t octetCountDiff = newOctetCount - fLastOctetCount;
  fLastOctetCount = newOctetCount;
  u_int32_t prevTotalOctetCount_lo = fTotalOctetCount_lo;
  fTotalOctetCount_lo += octetCountDiff;
  if (fTotalOctetCount_lo < prevTotalOctetCount_lo) { // wrap around
    ++fTotalOctetCount_hi;
  }

  u_int32_t newPacketCount = fOurRTPSink.packetCount();
  u_int32_t packetCountDiff = newPacketCount - fLastPacketCount;
  fLastPacketCount = newPacketCount;
  u_int32_t prevTotalPacketCount_lo = fTotalPacketCount_lo;
  fTotalPacketCount_lo += packetCountDiff;
  if (fTotalPacketCount_lo < prevTotalPacketCount_lo) { // wrap around
    ++fTotalPacketCount_hi;
  }
}

unsigned RTPTransmissionStats::roundTripDelay() const {
  // Compute the round-trip delay that was indicated by the most recently-received
  // RTCP RR packet.  Use the method noted in the RTP/RTCP specification (RFC 3350).

  if (fLastSRTime == 0) {
    // Either no RTCP RR packet has been received yet, or else the
    // reporting receiver has not yet received any RTCP SR packets from us:
    return 0;
  }

  // First, convert the time that we received the last RTCP RR packet to NTP format,
  // in units of 1/65536 (2^-16) seconds:
  unsigned lastReceivedTimeNTP_high
    = fTimeReceived.tv_sec + 0x83AA7E80; // 1970 epoch -> 1900 epoch
  double fractionalPart = (fTimeReceived.tv_usec*0x0400)/15625.0; // 2^16/10^6
  unsigned lastReceivedTimeNTP
    = (unsigned)((lastReceivedTimeNTP_high<<16) + fractionalPart + 0.5);

  int rawResult = lastReceivedTimeNTP - fLastSRTime - fDiffSR_RRTime;
  if (rawResult < 0) {
    // This can happen if there's clock drift between the sender and receiver,
    // and if the round-trip time was very small.
    rawResult = 0;
  }
  return (unsigned)rawResult;
}

void RTPTransmissionStats::getTotalOctetCount(u_int32_t& hi, u_int32_t& lo) {
  hi = fTotalOctetCount_hi;
  lo = fTotalOctetCount_lo;
}

void RTPTransmissionStats::getTotalPacketCount(u_int32_t& hi, u_int32_t& lo) {
  hi = fTotalPacketCount_hi;
  lo = fTotalPacketCount_lo;
}

unsigned RTPTransmissionStats::packetsReceivedSinceLastRR() const {
  if (!fOldValid) return 0;

  return fLastPacketNumReceived-fOldLastPacketNumReceived;
}

int RTPTransmissionStats::packetsLostBetweenRR() const {
  if (!fOldValid) return 0;

  return fTotNumPacketsLost - fOldTotNumPacketsLost;
}
