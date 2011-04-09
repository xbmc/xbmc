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
// RTP Sources
// Implementation

#include "RTPSource.hh"
#include "GroupsockHelper.hh"

////////// RTPSource //////////

Boolean RTPSource::lookupByName(UsageEnvironment& env,
				char const* sourceName,
				RTPSource*& resultSource) {
  resultSource = NULL; // unless we succeed

  MediaSource* source;
  if (!MediaSource::lookupByName(env, sourceName, source)) return False;

  if (!source->isRTPSource()) {
    env.setResultMsg(sourceName, " is not a RTP source");
    return False;
  }

  resultSource = (RTPSource*)source;
  return True;
}

Boolean RTPSource::hasBeenSynchronizedUsingRTCP() {
  return fCurPacketHasBeenSynchronizedUsingRTCP;
}

Boolean RTPSource::isRTPSource() const {
  return True;
}

RTPSource::RTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		     unsigned char rtpPayloadFormat,
		     u_int32_t rtpTimestampFrequency)
  : FramedSource(env),
    fRTPInterface(this, RTPgs),
    fCurPacketHasBeenSynchronizedUsingRTCP(False),
    fRTPPayloadFormat(rtpPayloadFormat),
    fTimestampFrequency(rtpTimestampFrequency),
    fSSRC(our_random32()) {
  fReceptionStatsDB = new RTPReceptionStatsDB();
}

RTPSource::~RTPSource() {
  delete fReceptionStatsDB;
}

void RTPSource::getAttributes() const {
  envir().setResultMsg(""); // Fix later to get attributes from  header #####
}


////////// RTPReceptionStatsDB //////////

RTPReceptionStatsDB::RTPReceptionStatsDB()
  : fTable(HashTable::create(ONE_WORD_HASH_KEYS)), fTotNumPacketsReceived(0) {
  reset();
}

void RTPReceptionStatsDB::reset() {
  fNumActiveSourcesSinceLastReset = 0;

  Iterator iter(*this);
  RTPReceptionStats* stats;
  while ((stats = iter.next()) != NULL) {
    stats->reset();
  }
}

RTPReceptionStatsDB::~RTPReceptionStatsDB() {
  // First, remove and delete all stats records from the table:
  RTPReceptionStats* stats;
  while ((stats = (RTPReceptionStats*)fTable->RemoveNext()) != NULL) {
    delete stats;
  }

  // Then, delete the table itself:
  delete fTable;
}

void RTPReceptionStatsDB
::noteIncomingPacket(u_int32_t SSRC, u_int16_t seqNum,
		     u_int32_t rtpTimestamp, unsigned timestampFrequency,
		     Boolean useForJitterCalculation,
		     struct timeval& resultPresentationTime,
		     Boolean& resultHasBeenSyncedUsingRTCP,
		     unsigned packetSize) {
  ++fTotNumPacketsReceived;
  RTPReceptionStats* stats = lookup(SSRC);
  if (stats == NULL) {
    // This is the first time we've heard from this SSRC.
    // Create a new record for it:
    stats = new RTPReceptionStats(SSRC, seqNum);
    if (stats == NULL) return;
    add(SSRC, stats);
  }

  if (stats->numPacketsReceivedSinceLastReset() == 0) {
    ++fNumActiveSourcesSinceLastReset;
  }

  stats->noteIncomingPacket(seqNum, rtpTimestamp, timestampFrequency,
			    useForJitterCalculation,
			    resultPresentationTime,
			    resultHasBeenSyncedUsingRTCP, packetSize);
}

void RTPReceptionStatsDB
::noteIncomingSR(u_int32_t SSRC,
		 u_int32_t ntpTimestampMSW, u_int32_t ntpTimestampLSW,
		 u_int32_t rtpTimestamp) {
  RTPReceptionStats* stats = lookup(SSRC);
  if (stats == NULL) {
    // This is the first time we've heard of this SSRC.
    // Create a new record for it:
    stats = new RTPReceptionStats(SSRC);
    if (stats == NULL) return;
    add(SSRC, stats);
  }

  stats->noteIncomingSR(ntpTimestampMSW, ntpTimestampLSW, rtpTimestamp);
}

void RTPReceptionStatsDB::removeRecord(u_int32_t SSRC) {
  RTPReceptionStats* stats = lookup(SSRC);
  if (stats != NULL) {
    long SSRC_long = (long)SSRC;
    fTable->Remove((char const*)SSRC_long);
    delete stats;
  }
}

RTPReceptionStatsDB::Iterator
::Iterator(RTPReceptionStatsDB& receptionStatsDB)
  : fIter(HashTable::Iterator::create(*(receptionStatsDB.fTable))) {
}

RTPReceptionStatsDB::Iterator::~Iterator() {
  delete fIter;
}

RTPReceptionStats*
RTPReceptionStatsDB::Iterator::next(Boolean includeInactiveSources) {
  char const* key; // dummy

  // If asked, skip over any sources that haven't been active
  // since the last reset:
  RTPReceptionStats* stats;
  do {
    stats = (RTPReceptionStats*)(fIter->next(key));
  } while (stats != NULL && !includeInactiveSources
	   && stats->numPacketsReceivedSinceLastReset() == 0);

  return stats;
}

RTPReceptionStats* RTPReceptionStatsDB::lookup(u_int32_t SSRC) const {
  long SSRC_long = (long)SSRC;
  return (RTPReceptionStats*)(fTable->Lookup((char const*)SSRC_long));
}

void RTPReceptionStatsDB::add(u_int32_t SSRC, RTPReceptionStats* stats) {
  long SSRC_long = (long)SSRC;
  fTable->Add((char const*)SSRC_long, stats);
}

////////// RTPReceptionStats //////////

RTPReceptionStats::RTPReceptionStats(u_int32_t SSRC, u_int16_t initialSeqNum) {
  initSeqNum(initialSeqNum);
  init(SSRC);
}

RTPReceptionStats::RTPReceptionStats(u_int32_t SSRC) {
  init(SSRC);
}

RTPReceptionStats::~RTPReceptionStats() {
}

void RTPReceptionStats::init(u_int32_t SSRC) {
  fSSRC = SSRC;
  fTotNumPacketsReceived = 0;
  fTotBytesReceived_hi = fTotBytesReceived_lo = 0;
  fHaveSeenInitialSequenceNumber = False;
  fLastTransit = ~0;
  fPreviousPacketRTPTimestamp = 0;
  fJitter = 0.0;
  fLastReceivedSR_NTPmsw = fLastReceivedSR_NTPlsw = 0;
  fLastReceivedSR_time.tv_sec = fLastReceivedSR_time.tv_usec = 0;
  fLastPacketReceptionTime.tv_sec = fLastPacketReceptionTime.tv_usec = 0;
  fMinInterPacketGapUS = 0x7FFFFFFF;
  fMaxInterPacketGapUS = 0;
  fTotalInterPacketGaps.tv_sec = fTotalInterPacketGaps.tv_usec = 0;
  fHasBeenSynchronized = False;
  fSyncTime.tv_sec = fSyncTime.tv_usec = 0;
  reset();
}

void RTPReceptionStats::initSeqNum(u_int16_t initialSeqNum) {
    fBaseExtSeqNumReceived = initialSeqNum-1;
    fHighestExtSeqNumReceived = initialSeqNum;
    fHaveSeenInitialSequenceNumber = True;
}

#ifndef MILLION
#define MILLION 1000000
#endif

void RTPReceptionStats
::noteIncomingPacket(u_int16_t seqNum, u_int32_t rtpTimestamp,
		     unsigned timestampFrequency,
		     Boolean useForJitterCalculation,
		     struct timeval& resultPresentationTime,
		     Boolean& resultHasBeenSyncedUsingRTCP,
		     unsigned packetSize) {
  if (!fHaveSeenInitialSequenceNumber) initSeqNum(seqNum);

  ++fNumPacketsReceivedSinceLastReset;
  ++fTotNumPacketsReceived;
  u_int32_t prevTotBytesReceived_lo = fTotBytesReceived_lo;
  fTotBytesReceived_lo += packetSize;
  if (fTotBytesReceived_lo < prevTotBytesReceived_lo) { // wrap-around
    ++fTotBytesReceived_hi;
  }

  // Check whether the new sequence number is the highest yet seen:
  unsigned oldSeqNum = (fHighestExtSeqNumReceived&0xFFFF);
  if (seqNumLT((u_int16_t)oldSeqNum, seqNum)) {
    // This packet was not an old packet received out of order, so check it:
    unsigned seqNumCycle = (fHighestExtSeqNumReceived&0xFFFF0000);
    unsigned seqNumDifference = (unsigned)((int)seqNum-(int)oldSeqNum);
    if (seqNumDifference >= 0x8000) {
      // The sequence number wrapped around, so start a new cycle:
      seqNumCycle += 0x10000;
    }

    unsigned newSeqNum = seqNumCycle|seqNum;
    if (newSeqNum > fHighestExtSeqNumReceived) {
      fHighestExtSeqNumReceived = newSeqNum;
    }
  }

  // Record the inter-packet delay
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  if (fLastPacketReceptionTime.tv_sec != 0
      || fLastPacketReceptionTime.tv_usec != 0) {
    unsigned gap
      = (timeNow.tv_sec - fLastPacketReceptionTime.tv_sec)*MILLION
      + timeNow.tv_usec - fLastPacketReceptionTime.tv_usec; 
    if (gap > fMaxInterPacketGapUS) {
      fMaxInterPacketGapUS = gap;
    }
    if (gap < fMinInterPacketGapUS) {
      fMinInterPacketGapUS = gap;
    }
    fTotalInterPacketGaps.tv_usec += gap;
    if (fTotalInterPacketGaps.tv_usec >= MILLION) {
      ++fTotalInterPacketGaps.tv_sec;
      fTotalInterPacketGaps.tv_usec -= MILLION;
    }
  }
  fLastPacketReceptionTime = timeNow;

  // Compute the current 'jitter' using the received packet's RTP timestamp,
  // and the RTP timestamp that would correspond to the current time.
  // (Use the code from appendix A.8 in the RTP spec.)
  // Note, however, that we don't use this packet if its timestamp is
  // the same as that of the previous packet (this indicates a multi-packet
  // fragment), or if we've been explicitly told not to use this packet.
  if (useForJitterCalculation
      && rtpTimestamp != fPreviousPacketRTPTimestamp) {
    unsigned arrival = (timestampFrequency*timeNow.tv_sec);
    arrival += (unsigned)
      ((2.0*timestampFrequency*timeNow.tv_usec + 1000000.0)/2000000);
            // note: rounding
    int transit = arrival - rtpTimestamp;
    if (fLastTransit == (~0)) fLastTransit = transit; // hack for first time
    int d = transit - fLastTransit;
    fLastTransit = transit;
    if (d < 0) d = -d;
    fJitter += (1.0/16.0) * ((double)d - fJitter);
  }

  // Return the 'presentation time' that corresponds to "rtpTimestamp":
  if (fSyncTime.tv_sec == 0 && fSyncTime.tv_usec == 0) {
    // This is the first timestamp that we've seen, so use the current
    // 'wall clock' time as the synchronization time.  (This will be
    // corrected later when we receive RTCP SRs.)
    fSyncTimestamp = rtpTimestamp;
    fSyncTime = timeNow;
  }

  int timestampDiff = rtpTimestamp - fSyncTimestamp;
      // Note: This works even if the timestamp wraps around
      // (as long as "int" is 32 bits)

  // Divide this by the timestamp frequency to get real time:
  double timeDiff = timestampDiff/(double)timestampFrequency;

  // Add this to the 'sync time' to get our result:
  unsigned const million = 1000000;
  unsigned seconds, uSeconds;
  if (timeDiff >= 0.0) {
    seconds = fSyncTime.tv_sec + (unsigned)(timeDiff);
    uSeconds = fSyncTime.tv_usec
      + (unsigned)((timeDiff - (unsigned)timeDiff)*million);
    if (uSeconds >= million) {
      uSeconds -= million;
      ++seconds;
    }
  } else {
    timeDiff = -timeDiff;
    seconds = fSyncTime.tv_sec - (unsigned)(timeDiff);
    uSeconds = fSyncTime.tv_usec
      - (unsigned)((timeDiff - (unsigned)timeDiff)*million);
    if ((int)uSeconds < 0) {
      uSeconds += million;
      --seconds;
    }
  }
  resultPresentationTime.tv_sec = seconds;
  resultPresentationTime.tv_usec = uSeconds;
  resultHasBeenSyncedUsingRTCP = fHasBeenSynchronized;

  // Save these as the new synchronization timestamp & time:
  fSyncTimestamp = rtpTimestamp;
  fSyncTime = resultPresentationTime;

  fPreviousPacketRTPTimestamp = rtpTimestamp;
}

void RTPReceptionStats::noteIncomingSR(u_int32_t ntpTimestampMSW,
				       u_int32_t ntpTimestampLSW,
				       u_int32_t rtpTimestamp) {
  fLastReceivedSR_NTPmsw = ntpTimestampMSW;
  fLastReceivedSR_NTPlsw = ntpTimestampLSW;

  gettimeofday(&fLastReceivedSR_time, NULL);

  // Use this SR to update time synchronization information:
  fSyncTimestamp = rtpTimestamp;
  fSyncTime.tv_sec = ntpTimestampMSW - 0x83AA7E80; // 1/1/1900 -> 1/1/1970
  double microseconds = (ntpTimestampLSW*15625.0)/0x04000000; // 10^6/2^32
  fSyncTime.tv_usec = (unsigned)(microseconds+0.5);
  fHasBeenSynchronized = True;
}

double RTPReceptionStats::totNumKBytesReceived() const {
  double const hiMultiplier = 0x20000000/125.0; // == (2^32)/(10^3)
  return fTotBytesReceived_hi*hiMultiplier + fTotBytesReceived_lo/1000.0;
}

unsigned RTPReceptionStats::jitter() const {
  return (unsigned)fJitter;
}

void RTPReceptionStats::reset() {
  fNumPacketsReceivedSinceLastReset = 0;
  fLastResetExtSeqNumReceived = fHighestExtSeqNumReceived;
}

Boolean seqNumLT(u_int16_t s1, u_int16_t s2) {
  // a 'less-than' on 16-bit sequence numbers
  int diff = s2-s1;
  if (diff > 0) {
    return (diff < 0x8000);
  } else if (diff < 0) {
    return (diff < -0x8000);
  } else { // diff == 0
    return False;
  }
}
