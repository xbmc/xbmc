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
// A class for generating MPEG-2 Transport Stream from one or more input
// Elementary Stream data sources
// Implementation

#include "MPEG2TransportStreamMultiplexor.hh"

#define TRANSPORT_PACKET_SIZE 188

#define PAT_FREQUENCY 100 // # of packets between Program Association Tables
#define PMT_FREQUENCY 500 // # of packets between Program Map Tables

#define PID_TABLE_SIZE 256

MPEG2TransportStreamMultiplexor
::MPEG2TransportStreamMultiplexor(UsageEnvironment& env)
  : FramedSource(env),
    fHaveVideoStreams(True/*by default*/),
    fOutgoingPacketCounter(0), fProgramMapVersion(0),
    fPreviousInputProgramMapVersion(0xFF), fCurrentInputProgramMapVersion(0xFF),
    fPCR_PID(0), fCurrentPID(0),
    fInputBuffer(NULL), fInputBufferSize(0), fInputBufferBytesUsed(0),
    fIsFirstAdaptationField(True) {
  for (unsigned i = 0; i < PID_TABLE_SIZE; ++i) {
    fPIDState[i].counter = 0;
    fPIDState[i].streamType = 0;
  }
}

MPEG2TransportStreamMultiplexor::~MPEG2TransportStreamMultiplexor() {
}

void MPEG2TransportStreamMultiplexor::doGetNextFrame() {
  if (fInputBufferBytesUsed >= fInputBufferSize) {
    // No more bytes are available from the current buffer.
    // Arrange to read a new one.
    awaitNewBuffer(fInputBuffer);
    return;
  }

  do {
    // Periodically return a Program Association Table packet instead:
    if (fOutgoingPacketCounter++ % PAT_FREQUENCY == 0) {
      deliverPATPacket();
      break;
    }

    // Periodically (or when we see a new PID) return a Program Map Table instead:
    Boolean programMapHasChanged = fPIDState[fCurrentPID].counter == 0
      || fCurrentInputProgramMapVersion != fPreviousInputProgramMapVersion;
    if (fOutgoingPacketCounter % PMT_FREQUENCY == 0 || programMapHasChanged) {
      if (programMapHasChanged) { // reset values for next time:
	fPIDState[fCurrentPID].counter = 1;
	fPreviousInputProgramMapVersion = fCurrentInputProgramMapVersion;
      }
      deliverPMTPacket(programMapHasChanged);
      break;
    }

    // Normal case: Deliver (or continue delivering) the recently-read data:
    deliverDataToClient(fCurrentPID, fInputBuffer, fInputBufferSize,
			fInputBufferBytesUsed);
  } while (0);

  // NEED TO SET fPresentationTime, durationInMicroseconds #####
  // Complete the delivery to the client:
  afterGetting(this);
}

void MPEG2TransportStreamMultiplexor
::handleNewBuffer(unsigned char* buffer, unsigned bufferSize,
		  int mpegVersion, MPEG1or2Demux::SCR scr) {
  if (bufferSize < 4) return;
  fInputBuffer = buffer;
  fInputBufferSize = bufferSize;
  fInputBufferBytesUsed = 0;

  u_int8_t stream_id = fInputBuffer[3];
  // Use "stream_id" directly as our PID.
  // Also, figure out the Program Map 'stream type' from this.
  if (stream_id == 0xBE) { // padding_stream; ignore
    fInputBufferSize = 0;
  } else if (stream_id == 0xBC) { // program_stream_map
    setProgramStreamMap(fInputBufferSize);
    fInputBufferSize = 0; // then, ignore the buffer
  } else {
    fCurrentPID = stream_id;

    // Set the stream's type:
    u_int8_t& streamType = fPIDState[fCurrentPID].streamType; // alias

    if (streamType == 0) {
      // Instead, set the stream's type to default values, based on whether
      // the stream is audio or video, and whether it's MPEG-1 or MPEG-2:
      if ((stream_id&0xF0) == 0xE0) { // video
	streamType = mpegVersion == 1 ? 1 : mpegVersion == 2 ? 2 : mpegVersion == 4 ? 0x10 : 0x1B;
      } else if ((stream_id&0xE0) == 0xC0) { // audio
	streamType = mpegVersion == 1 ? 3 : mpegVersion == 2 ? 4 : 0xF;
      } else if (stream_id == 0xBD) { // private_stream1 (usually AC-3)
	streamType = 0x06; // for DVB; for ATSC, use 0x81
      } else { // something else, e.g., AC-3 uses private_stream1 (0xBD)
	streamType = 0x81; // private
      }
    }

    if (fPCR_PID == 0) { // set it to this stream, if it's appropriate:
      if ((!fHaveVideoStreams && (streamType == 3 || streamType == 4 || streamType == 0xF))/* audio stream */ ||
	  (streamType == 1 || streamType == 2 || streamType == 0x10 || streamType == 0x1B)/* video stream */) {
	fPCR_PID = fCurrentPID; // use this stream's SCR for PCR
      }
    }
    if (fCurrentPID == fPCR_PID) {
      // Record the input's current SCR timestamp, for use as our PCR:
      fPCR = scr;
    }
  }

  // Now that we have new input data, retry the last delivery to the client:
  doGetNextFrame();
}

void MPEG2TransportStreamMultiplexor
::deliverDataToClient(u_int8_t pid, unsigned char* buffer, unsigned bufferSize,
		      unsigned& startPositionInBuffer) {
  // Construct a new Transport packet, and deliver it to the client:
  if (fMaxSize < TRANSPORT_PACKET_SIZE) {
    fFrameSize = 0; // the client hasn't given us enough space; deliver nothing
    fNumTruncatedBytes = TRANSPORT_PACKET_SIZE;
  } else {
    fFrameSize = TRANSPORT_PACKET_SIZE;
    Boolean willAddPCR = pid == fPCR_PID && startPositionInBuffer == 0
      && !(fPCR.highBit == 0 && fPCR.remainingBits == 0 && fPCR.extension == 0);
    unsigned const numBytesAvailable = bufferSize - startPositionInBuffer;
    unsigned numHeaderBytes = 4; // by default
    unsigned numPCRBytes = 0; // by default
    unsigned numPaddingBytes = 0; // by default
    unsigned numDataBytes;
    u_int8_t adaptation_field_control;
    if (willAddPCR) {
      adaptation_field_control = 0x30;
      numHeaderBytes += 2; // for the "adaptation_field_length" and flags
      numPCRBytes = 6;
      if (numBytesAvailable >= TRANSPORT_PACKET_SIZE - numHeaderBytes - numPCRBytes) {
	numDataBytes = TRANSPORT_PACKET_SIZE - numHeaderBytes - numPCRBytes;
      } else {
	numDataBytes = numBytesAvailable;
	numPaddingBytes
	  = TRANSPORT_PACKET_SIZE - numHeaderBytes - numPCRBytes - numDataBytes;
      }
    } else if (numBytesAvailable >= TRANSPORT_PACKET_SIZE - numHeaderBytes) {
      // This is the common case
      adaptation_field_control = 0x10;
      numDataBytes = TRANSPORT_PACKET_SIZE - numHeaderBytes;
    } else {
      adaptation_field_control = 0x30;
      ++numHeaderBytes; // for the "adaptation_field_length"
      // ASSERT: numBytesAvailable <= TRANSPORT_PACKET_SIZE - numHeaderBytes
      numDataBytes = numBytesAvailable;
      if (numDataBytes < TRANSPORT_PACKET_SIZE - numHeaderBytes) {
	++numHeaderBytes; // for the adaptation field flags
	numPaddingBytes = TRANSPORT_PACKET_SIZE - numHeaderBytes - numDataBytes;
      }
    }
    // ASSERT: numHeaderBytes+numPCRBytes+numPaddingBytes+numDataBytes
    //         == TRANSPORT_PACKET_SIZE

    // Fill in the header of the Transport Stream packet:
    unsigned char* header = fTo;
    *header++ = 0x47; // sync_byte
    *header++ = (startPositionInBuffer == 0) ? 0x40 : 0x00;
      // transport_error_indicator, payload_unit_start_indicator, transport_priority,
      // first 5 bits of PID
    *header++ = pid;
      // last 8 bits of PID
    unsigned& continuity_counter = fPIDState[pid].counter; // alias
    *header++ = adaptation_field_control|(continuity_counter&0x0F);
      // transport_scrambling_control, adaptation_field_control, continuity_counter
    ++continuity_counter;
    if (adaptation_field_control == 0x30) {
      // Add an adaptation field:
      u_int8_t adaptation_field_length
	= (numHeaderBytes == 5) ? 0 : 1 + numPCRBytes + numPaddingBytes;
      *header++ = adaptation_field_length;
      if (numHeaderBytes > 5) {
	u_int8_t flags = willAddPCR ? 0x10 : 0x00;
	if (fIsFirstAdaptationField) {
	  flags |= 0x80; // discontinuity_indicator
	  fIsFirstAdaptationField = False;
	}
	*header++ = flags;
	if (willAddPCR) {
	  u_int32_t pcrHigh32Bits = (fPCR.highBit<<31) | (fPCR.remainingBits>>1);
	  u_int8_t pcrLowBit = fPCR.remainingBits&1;
	  u_int8_t extHighBit = (fPCR.extension&0x100)>>8;
	  *header++ = pcrHigh32Bits>>24;
	  *header++ = pcrHigh32Bits>>16;
	  *header++ = pcrHigh32Bits>>8;
	  *header++ = pcrHigh32Bits;
	  *header++ = (pcrLowBit<<7)|0x7E|extHighBit;
	  *header++ = (u_int8_t)fPCR.extension; // low 8 bits of extension
	}
      }
    }

    // Add any padding bytes:
    for (unsigned i = 0; i < numPaddingBytes; ++i) *header++ = 0xFF;

    // Finally, add the data bytes:
    memmove(header, &buffer[startPositionInBuffer], numDataBytes);
    startPositionInBuffer += numDataBytes;
  }
}

static u_int32_t calculateCRC(u_int8_t* data, unsigned dataLength); // forward

#define PAT_PID 0
#define OUR_PROGRAM_NUMBER 1
#define OUR_PROGRAM_MAP_PID 0x10

void MPEG2TransportStreamMultiplexor::deliverPATPacket() {
  // First, create a new buffer for the PAT packet:
  unsigned const patSize = TRANSPORT_PACKET_SIZE - 4; // allow for the 4-byte header
  unsigned char* patBuffer = new unsigned char[patSize];

  // and fill it in:
  unsigned char* pat = patBuffer;
  *pat++ = 0; // pointer_field
  *pat++ = 0; // table_id
  *pat++ = 0xB0; // section_syntax_indicator; 0; reserved, section_length (high)
  *pat++ = 13; // section_length (low)
  *pat++ = 0; *pat++ = 1; // transport_stream_id
  *pat++ = 0xC3; // reserved; version_number; current_next_indicator
  *pat++ = 0; // section_number
  *pat++ = 0; // last_section_number
  *pat++ = OUR_PROGRAM_NUMBER>>8; *pat++ = OUR_PROGRAM_NUMBER; // program_number
  *pat++ = 0xE0|(OUR_PROGRAM_MAP_PID>>8); // reserved; program_map_PID (high)
  *pat++ = OUR_PROGRAM_MAP_PID; // program_map_PID (low)

  // Compute the CRC from the bytes we currently have (not including "pointer_field"):
  u_int32_t crc = calculateCRC(patBuffer+1, pat - (patBuffer+1));
  *pat++ = crc>>24; *pat++ = crc>>16; *pat++ = crc>>8; *pat++ = crc;

  // Fill in the rest of the packet with padding bytes:
  while (pat < &patBuffer[patSize]) *pat++ = 0xFF;

  // Deliver the packet:
  unsigned startPosition = 0;
  deliverDataToClient(PAT_PID, patBuffer, patSize, startPosition);

  // Finally, remove the new buffer:
  delete[] patBuffer;
}

void MPEG2TransportStreamMultiplexor::deliverPMTPacket(Boolean hasChanged) {
  if (hasChanged) ++fProgramMapVersion;

  // First, create a new buffer for the PMT packet:
  unsigned const pmtSize = TRANSPORT_PACKET_SIZE - 4; // allow for the 4-byte header
  unsigned char* pmtBuffer = new unsigned char[pmtSize];

  // and fill it in:
  unsigned char* pmt = pmtBuffer;
  *pmt++ = 0; // pointer_field
  *pmt++ = 2; // table_id
  *pmt++ = 0xB0; // section_syntax_indicator; 0; reserved, section_length (high)
  unsigned char* section_lengthPtr = pmt; // save for later
  *pmt++ = 0; // section_length (low) (fill in later)
  *pmt++ = OUR_PROGRAM_NUMBER>>8; *pmt++ = OUR_PROGRAM_NUMBER; // program_number
  *pmt++ = 0xC1|((fProgramMapVersion&0x1F)<<1); // reserved; version_number; current_next_indicator
  *pmt++ = 0; // section_number
  *pmt++ = 0; // last_section_number
  *pmt++ = 0xE0; // reserved; PCR_PID (high)
  *pmt++ = fPCR_PID; // PCR_PID (low)
  *pmt++ = 0xF0; // reserved; program_info_length (high)
  *pmt++ = 0; // program_info_length (low)
  for (int pid = 0; pid < PID_TABLE_SIZE; ++pid) {
    if (fPIDState[pid].streamType != 0) {
      // This PID gets recorded in the table
      *pmt++ = fPIDState[pid].streamType;
      *pmt++ = 0xE0; // reserved; elementary_pid (high)
      *pmt++ = pid; // elementary_pid (low)
      *pmt++ = 0xF0; // reserved; ES_info_length (high)
      *pmt++ = 0; // ES_info_length (low)
    }
  }
  unsigned section_length = pmt - (section_lengthPtr+1) + 4 /*for CRC*/;
  *section_lengthPtr = section_length;

  // Compute the CRC from the bytes we currently have (not including "pointer_field"):
  u_int32_t crc = calculateCRC(pmtBuffer+1, pmt - (pmtBuffer+1));
  *pmt++ = crc>>24; *pmt++ = crc>>16; *pmt++ = crc>>8; *pmt++ = crc;

  // Fill in the rest of the packet with padding bytes:
  while (pmt < &pmtBuffer[pmtSize]) *pmt++ = 0xFF;

  // Deliver the packet:
  unsigned startPosition = 0;
  deliverDataToClient(OUR_PROGRAM_MAP_PID, pmtBuffer, pmtSize, startPosition);

  // Finally, remove the new buffer:
  delete[] pmtBuffer;
}

void MPEG2TransportStreamMultiplexor::setProgramStreamMap(unsigned frameSize) {
  if (frameSize <= 16) return; // program_stream_map is too small to be useful
  if (frameSize > 0xFF) return; // program_stream_map is too large

  u_int16_t program_stream_map_length = (fInputBuffer[4]<<8) | fInputBuffer[5];
  if ((u_int16_t)frameSize > 6+program_stream_map_length) {
    frameSize = 6+program_stream_map_length;
  }

  u_int8_t versionByte = fInputBuffer[6];
  if ((versionByte&0x80) == 0) return; // "current_next_indicator" is not set
  fCurrentInputProgramMapVersion = versionByte&0x1F;

  u_int16_t program_stream_info_length = (fInputBuffer[8]<<8) | fInputBuffer[9];
  unsigned offset = 10 + program_stream_info_length; // skip over 'descriptors'

  u_int16_t elementary_stream_map_length
    = (fInputBuffer[offset]<<8) | fInputBuffer[offset+1];
  offset += 2;
  frameSize -= 4; // sizeof CRC_32
  if (frameSize > offset + elementary_stream_map_length) {
    frameSize = offset + elementary_stream_map_length;
  }

  while (offset + 4 <= frameSize) {
    u_int8_t stream_type = fInputBuffer[offset];
    u_int8_t elementary_stream_id = fInputBuffer[offset+1];

    fPIDState[elementary_stream_id].streamType = stream_type;

    u_int16_t elementary_stream_info_length
      = (fInputBuffer[offset+2]<<8) | fInputBuffer[offset+3];
    offset += 4 + elementary_stream_info_length;
  }
}

static u_int32_t CRC32[256] = {
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static u_int32_t calculateCRC(u_int8_t* data, unsigned dataLength) {
  u_int32_t crc = 0xFFFFFFFF;

  while (dataLength-- > 0) {
    crc = (crc<<8) ^ CRC32[(crc>>24) ^ (u_int32_t)(*data++)];
  }

  return crc;
}
