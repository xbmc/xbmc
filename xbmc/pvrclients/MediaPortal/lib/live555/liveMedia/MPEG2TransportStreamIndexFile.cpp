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
// A class that encapsulates MPEG-2 Transport Stream 'index files'/
// These index files are used to implement 'trick play' operations
// (seek-by-time, fast forward, reverse play) on Transport Stream files.
//
// Implementation

#include "MPEG2TransportStreamIndexFile.hh"
#include "InputFile.hh"

MPEG2TransportStreamIndexFile
::MPEG2TransportStreamIndexFile(UsageEnvironment& env, char const* indexFileName)
  : Medium(env),
    fFileName(strDup(indexFileName)), fFid(NULL), fCurrentIndexRecordNum(0),
    fCachedPCR(0.0f), fCachedTSPacketNumber(0), fNumIndexRecords(0) {
  // Get the file size, to determine how many index records it contains:
  u_int64_t indexFileSize = GetFileSize(indexFileName, NULL);
  if (indexFileSize % INDEX_RECORD_SIZE != 0) {
    env << "Warning: Size of the index file \"" << indexFileName
 	<< "\" (" << (unsigned)indexFileSize
	<< ") is not a multiple of the index record size ("
	<< INDEX_RECORD_SIZE << ")\n";
  }
  fNumIndexRecords = (unsigned long)(indexFileSize/INDEX_RECORD_SIZE);
}

MPEG2TransportStreamIndexFile* MPEG2TransportStreamIndexFile
::createNew(UsageEnvironment& env, char const* indexFileName) {
  if (indexFileName == NULL) return NULL;
  MPEG2TransportStreamIndexFile* indexFile
    = new MPEG2TransportStreamIndexFile(env, indexFileName);

  // Reject empty or non-existent index files:
  if (indexFile->getPlayingDuration() == 0.0f) {
    delete indexFile;
    indexFile = NULL;
  }

  return indexFile;
}

MPEG2TransportStreamIndexFile::~MPEG2TransportStreamIndexFile() {
  closeFid();
  delete[] fFileName;
}

void MPEG2TransportStreamIndexFile
::lookupTSPacketNumFromNPT(float& npt, unsigned long& tsPacketNumber,
			   unsigned long& indexRecordNumber) {
  if (npt <= 0.0 || fNumIndexRecords == 0) { // Fast-track a common case:
    npt = 0.0f;
    tsPacketNumber = indexRecordNumber = 0;
    return;
  }

  // If "npt" is the same as the one that we last looked up, return its cached result:
  if (npt == fCachedPCR) {
    tsPacketNumber = fCachedTSPacketNumber;
    indexRecordNumber = fCachedIndexRecordNumber;
    return;
  }

  // Search for the pair of neighboring index records whose PCR values span "npt".
  // Use the 'regula-falsi' method.
  Boolean success = False;
  unsigned long ixFound = 0;
  do {
    unsigned long ixLeft = 0, ixRight = fNumIndexRecords-1;
    float pcrLeft = 0.0f, pcrRight;
    if (!readIndexRecord(ixRight)) break;
    pcrRight = pcrFromBuf();
    if (npt > pcrRight) npt = pcrRight;
        // handle "npt" too large by seeking to the last frame of the file

    while (ixRight-ixLeft > 1 && pcrLeft < npt && npt <= pcrRight) {
      unsigned long ixNew = ixLeft
	+ (unsigned long)(((npt-pcrLeft)/(pcrRight-pcrLeft))*(ixRight-ixLeft));
      if (ixNew == ixLeft || ixNew == ixRight) {
	// use bisection instead:
	ixNew = (ixLeft+ixRight)/2;
      }
      if (!readIndexRecord(ixNew)) break;
      float pcrNew = pcrFromBuf();
      if (pcrNew < npt) {
	pcrLeft = pcrNew;
	ixLeft = ixNew;
      } else {
	pcrRight = pcrNew;
	ixRight = ixNew;
      }
    }
    if (ixRight-ixLeft > 1 || npt <= pcrLeft || npt > pcrRight) break; // bad PCR values in index file?

    ixFound = ixRight;
    // "Rewind' until we reach the start of a Video Sequence or GOP header:
    success = rewindToVSH(ixFound);
  } while (0);

  if (success && readIndexRecord(ixFound)) {
    // Return (and cache) information from record "ixFound":
    npt = fCachedPCR = pcrFromBuf();
    tsPacketNumber = fCachedTSPacketNumber = tsPacketNumFromBuf();
    indexRecordNumber = fCachedIndexRecordNumber = ixFound;
  } else {
    // An error occurred: Return the default values, for npt == 0:
    npt = 0.0f;
    tsPacketNumber = indexRecordNumber = 0;
  }
  closeFid();
}

void MPEG2TransportStreamIndexFile
::lookupPCRFromTSPacketNum(unsigned long& tsPacketNumber, Boolean reverseToPreviousVSH,
			   float& pcr, unsigned long& indexRecordNumber) {
  if (tsPacketNumber == 0 || fNumIndexRecords == 0) { // Fast-track a common case:
    pcr = 0.0f;
    indexRecordNumber = 0;
    return;
  }

  // If "tsPacketNumber" is the same as the one that we last looked up, return its cached result:
  if (tsPacketNumber == fCachedTSPacketNumber) {
    pcr = fCachedPCR;
    indexRecordNumber = fCachedIndexRecordNumber;
    return;
  }

  // Search for the pair of neighboring index records whose TS packet #s span "tsPacketNumber".
  // Use the 'regula-falsi' method.
  Boolean success = False;
  unsigned long ixFound = 0;
  do {
    unsigned long ixLeft = 0, ixRight = fNumIndexRecords-1;
    unsigned long tsLeft = 0, tsRight;
    if (!readIndexRecord(ixRight)) break;
    tsRight = tsPacketNumFromBuf();
    if (tsPacketNumber > tsRight) tsPacketNumber = tsRight;
        // handle "tsPacketNumber" too large by seeking to the last frame of the file

    while (ixRight-ixLeft > 1 && tsLeft < tsPacketNumber && tsPacketNumber <= tsRight) {
      unsigned long ixNew = ixLeft
	+ (unsigned long)(((tsPacketNumber-tsLeft)/(tsRight-tsLeft))*(ixRight-ixLeft));
      if (ixNew == ixLeft || ixNew == ixRight) {
	// Use bisection instead:
	ixNew = (ixLeft+ixRight)/2;
      }
      if (!readIndexRecord(ixNew)) break;
      unsigned long tsNew = tsPacketNumFromBuf();
      if (tsNew < tsPacketNumber) {
	tsLeft = tsNew;
	ixLeft = ixNew;
      } else {
	tsRight = tsNew;
	ixRight = ixNew;
      }
    }
    if (ixRight-ixLeft > 1 || tsPacketNumber <= tsLeft || tsPacketNumber > tsRight) break; // bad PCR values in index file?

    ixFound = ixRight;
    if (reverseToPreviousVSH) {
      // "Rewind' until we reach the start of a Video Sequence or GOP header:
      success = rewindToVSH(ixFound);
    } else {
      success = True;
    }
  } while (0);

  if (success && readIndexRecord(ixFound)) {
    // Return (and cache) information from record "ixFound":
    pcr = fCachedPCR = pcrFromBuf();
    fCachedTSPacketNumber = tsPacketNumFromBuf();
    if (reverseToPreviousVSH) tsPacketNumber = fCachedTSPacketNumber;
    indexRecordNumber = fCachedIndexRecordNumber = ixFound;
  } else {
    // An error occurred: Return the default values, for tsPacketNumber == 0:
    pcr = 0.0f;
    indexRecordNumber = 0;
  }
  closeFid();
}

Boolean MPEG2TransportStreamIndexFile
::readIndexRecordValues(unsigned long indexRecordNum,
			unsigned long& transportPacketNum, u_int8_t& offset,
			u_int8_t& size, float& pcr, u_int8_t& recordType) {
  if (!readIndexRecord(indexRecordNum)) return False;

  transportPacketNum = tsPacketNumFromBuf();
  offset = offsetFromBuf();
  size = sizeFromBuf();
  pcr = pcrFromBuf();
  recordType = recordTypeFromBuf();
  return True;
}

float MPEG2TransportStreamIndexFile::getPlayingDuration() {
  if (fNumIndexRecords == 0 || !readOneIndexRecord(fNumIndexRecords-1)) return 0.0f;

  return pcrFromBuf();
}

Boolean MPEG2TransportStreamIndexFile::openFid() {
  if (fFid == NULL && fFileName != NULL) {
    if ((fFid = OpenInputFile(envir(), fFileName)) != NULL) {
      fCurrentIndexRecordNum = 0;
    }
  }

  return fFid != NULL;
}

Boolean MPEG2TransportStreamIndexFile::seekToIndexRecord(unsigned long indexRecordNumber) {
  if (!openFid()) return False;

  if (indexRecordNumber == fCurrentIndexRecordNum) return True; // we're already there

  if (SeekFile64(fFid, (int64_t)(indexRecordNumber*INDEX_RECORD_SIZE), SEEK_SET) != 0) return False;
  fCurrentIndexRecordNum = indexRecordNumber;
  return True;
}

Boolean MPEG2TransportStreamIndexFile::readIndexRecord(unsigned long indexRecordNum) {
  do {
    if (!seekToIndexRecord(indexRecordNum)) break;
    if (fread(fBuf, INDEX_RECORD_SIZE, 1, fFid) != 1) break;
    ++fCurrentIndexRecordNum;

    return True;
  } while (0);

  return False; // an error occurred
}

Boolean MPEG2TransportStreamIndexFile::readOneIndexRecord(unsigned long indexRecordNum) {
  Boolean result = readIndexRecord(indexRecordNum);
  closeFid();

  return result;
}

void MPEG2TransportStreamIndexFile::closeFid() {
  if (fFid != NULL) {
    CloseInputFile(fFid);
    fFid = NULL;
  }
}

float MPEG2TransportStreamIndexFile::pcrFromBuf() {
  unsigned pcr_int = (fBuf[5]<<16) | (fBuf[4]<<8) | fBuf[3];
  u_int8_t pcr_frac = fBuf[6];
  return pcr_int + pcr_frac/256.0f;
}

unsigned long MPEG2TransportStreamIndexFile::tsPacketNumFromBuf() {
  return (fBuf[10]<<24) | (fBuf[9]<<16) | (fBuf[8]<<8) | fBuf[7];
}

Boolean MPEG2TransportStreamIndexFile::rewindToVSH(unsigned long&ixFound) {
  Boolean success = False;

  while (ixFound > 0) {
    if (!readIndexRecord(ixFound)) break;

    u_int8_t recordType = recordTypeFromBuf();
    if ((recordType&0x80) != 0 && (recordType&0x7F) <= 2/*GOP*/) {
      if ((recordType&0x7F) == 2) {
	// This is a GOP.  Hack: If the preceding record is for a Video Sequence Header,
	// then use it instead:
	unsigned long newIxFound = ixFound;

	while (--newIxFound > 0) {
	  if (!readIndexRecord(newIxFound)) break;
	  recordType = recordTypeFromBuf();
	  if ((recordType&0x7F) != 1) break; // not a Video Sequence Header
	  if ((recordType&0x80) != 0) { // this is the start of the VSH; use it
	    ixFound = newIxFound;
	    break;
	  }
	}
      }
      // Record 'ixFound' as appropriate to return:
      success = True;
      break;
    }
    --ixFound;
  }
  if (ixFound == 0) success = True; // use record 0 anyway

  return success;
}
