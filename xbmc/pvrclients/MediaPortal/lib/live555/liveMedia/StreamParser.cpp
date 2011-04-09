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
// Abstract class for parsing a byte stream
// Implementation

#include "StreamParser.hh"

#include <string.h>
#include <stdlib.h>

#define BANK_SIZE 150000

StreamParser::StreamParser(FramedSource* inputSource,
			   FramedSource::onCloseFunc* onInputCloseFunc,
			   void* onInputCloseClientData,
			   clientContinueFunc* clientContinueFunc,
			   void* clientContinueClientData)
  : fInputSource(inputSource), fOnInputCloseFunc(onInputCloseFunc),
    fOnInputCloseClientData(onInputCloseClientData),
    fClientContinueFunc(clientContinueFunc),
    fClientContinueClientData(clientContinueClientData),
    fSavedParserIndex(0), fSavedRemainingUnparsedBits(0),
    fCurParserIndex(0), fRemainingUnparsedBits(0),
    fTotNumValidBytes(0) {
  fBank[0] = new unsigned char[BANK_SIZE];
  fBank[1] = new unsigned char[BANK_SIZE];
  fCurBankNum = 0;
  fCurBank = fBank[fCurBankNum];
}

StreamParser::~StreamParser() {
  delete[] fBank[0]; delete[] fBank[1];
}

#define NO_MORE_BUFFERED_INPUT 1

void StreamParser::ensureValidBytes1(unsigned numBytesNeeded) {
  // We need to read some more bytes from the input source.
  // First, clarify how much data to ask for:
  unsigned maxInputFrameSize = fInputSource->maxFrameSize();
  if (maxInputFrameSize > numBytesNeeded) numBytesNeeded = maxInputFrameSize;

  // First, check whether these new bytes would overflow the current
  // bank.  If so, start using a new bank now.
  if (fCurParserIndex + numBytesNeeded > BANK_SIZE) {
    // Swap banks, but save any still-needed bytes from the old bank:
    unsigned numBytesToSave = fTotNumValidBytes - fSavedParserIndex;
    unsigned char const* from = &curBank()[fSavedParserIndex];

    fCurBankNum = (fCurBankNum + 1)%2;
    fCurBank = fBank[fCurBankNum];
    memmove(curBank(), from, numBytesToSave);
    fCurParserIndex = fCurParserIndex - fSavedParserIndex;
    fSavedParserIndex = 0;
    fTotNumValidBytes = numBytesToSave;
  }

  // ASSERT: fCurParserIndex + numBytesNeeded > fTotNumValidBytes
  //      && fCurParserIndex + numBytesNeeded <= BANK_SIZE
  if (fCurParserIndex + numBytesNeeded > BANK_SIZE) {
    // If this happens, it means that we have too much saved parser state.
    // To fix this, increase BANK_SIZE as appropriate.
    fInputSource->envir() << "StreamParser internal error ("
			  << fCurParserIndex << "+ "
			  << numBytesNeeded << " > "
			  << BANK_SIZE << ")\n";
    abort();
  }

  // Try to read as many new bytes as will fit in the current bank:
  unsigned maxNumBytesToRead = BANK_SIZE - fTotNumValidBytes;
  fInputSource->getNextFrame(&curBank()[fTotNumValidBytes],
			     maxNumBytesToRead,
			     afterGettingBytes, this,
			     fOnInputCloseFunc, fOnInputCloseClientData);

  throw NO_MORE_BUFFERED_INPUT;
}

void StreamParser::afterGettingBytes(void* clientData,
				     unsigned numBytesRead,
				     unsigned /*numTruncatedBytes*/,
				     struct timeval presentationTime,
				     unsigned /*durationInMicroseconds*/){
  StreamParser* buffer = (StreamParser*)clientData;

  // Sanity check: Make sure we didn't get too many bytes for our bank:
  if (buffer->fTotNumValidBytes + numBytesRead > BANK_SIZE) {
    buffer->fInputSource->envir()
      << "StreamParser::afterGettingBytes() warning: read "
      << numBytesRead << " bytes; expected no more than "
      << BANK_SIZE - buffer->fTotNumValidBytes << "\n";
  }

  unsigned char* ptr = &buffer->curBank()[buffer->fTotNumValidBytes];
  buffer->fTotNumValidBytes += numBytesRead;

  // Continue our original calling source where it left off:
  buffer->restoreSavedParserState();
      // Sigh... this is a crock; things would have been a lot simpler
      // here if we were using threads, with synchronous I/O...
  buffer->fClientContinueFunc(buffer->fClientContinueClientData,
			      ptr, numBytesRead, presentationTime);
}

void StreamParser::saveParserState() {
  fSavedParserIndex = fCurParserIndex;
  fSavedRemainingUnparsedBits = fRemainingUnparsedBits;
}

void StreamParser::restoreSavedParserState() {
  fCurParserIndex = fSavedParserIndex;
  fRemainingUnparsedBits = fSavedRemainingUnparsedBits;
}

void StreamParser::skipBits(unsigned numBits) {
  if (numBits <= fRemainingUnparsedBits) {
    fRemainingUnparsedBits -= numBits;
  } else {
    numBits -= fRemainingUnparsedBits;

    unsigned numBytesToExamine = (numBits+7)/8; // round up
    ensureValidBytes(numBytesToExamine);
    fCurParserIndex += numBytesToExamine;

    fRemainingUnparsedBits = 8*numBytesToExamine - numBits;
  }
}

unsigned StreamParser::getBits(unsigned numBits) {
  if (numBits <= fRemainingUnparsedBits) {
    unsigned char lastByte = *lastParsed();
    lastByte >>= (fRemainingUnparsedBits - numBits);
    fRemainingUnparsedBits -= numBits;

    return (unsigned)lastByte &~ ((~0)<<numBits);
  } else {
    unsigned char lastByte;
    if (fRemainingUnparsedBits > 0) {
      lastByte = *lastParsed();
    } else {
      lastByte = 0;
    }

    unsigned remainingBits = numBits - fRemainingUnparsedBits; // > 0

    // For simplicity, read the next 4 bytes, even though we might not
    // need all of them here:
    unsigned result = test4Bytes();

    result >>= (32 - remainingBits);
    result |= (lastByte << remainingBits);
    if (numBits < 32) result &=~ ((~0)<<numBits);

    unsigned const numRemainingBytes = (remainingBits+7)/8;
    fCurParserIndex += numRemainingBytes;
    fRemainingUnparsedBits = 8*numRemainingBytes - remainingBits;

    return result;
  }
}

void StreamParser::flushInput() {
  fCurParserIndex = fSavedParserIndex = 0;
  fSavedRemainingUnparsedBits = fRemainingUnparsedBits = 0;
  fTotNumValidBytes = 0;
}
