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
// JPEG Video (RFC 2435) RTP Sources
// Implementation

#include "JPEGVideoRTPSource.hh"

////////// JPEGBufferedPacket and JPEGBufferedPacketFactory //////////

class JPEGBufferedPacket: public BufferedPacket {
public:
  Boolean completesFrame;

private:
  // Redefined virtual functions:
  virtual void reset();
  virtual unsigned nextEnclosedFrameSize(unsigned char*& framePtr,
					 unsigned dataSize);
};

class JPEGBufferedPacketFactory: public BufferedPacketFactory {
private: // redefined virtual functions
  virtual BufferedPacket* createNewPacket(MultiFramedRTPSource* ourSource);
};


////////// JPEGVideoRTPSource implementation //////////

#define BYTE unsigned char
#define WORD unsigned
#define DWORD unsigned long

JPEGVideoRTPSource*
JPEGVideoRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			      unsigned char rtpPayloadFormat,
			      unsigned rtpTimestampFrequency,
			      unsigned defaultWidth, unsigned defaultHeight) {
  return new JPEGVideoRTPSource(env, RTPgs, rtpPayloadFormat,
				rtpTimestampFrequency, defaultWidth, defaultHeight);
}

JPEGVideoRTPSource::JPEGVideoRTPSource(UsageEnvironment& env,
				       Groupsock* RTPgs,
				       unsigned char rtpPayloadFormat,
				       unsigned rtpTimestampFrequency,
				       unsigned defaultWidth, unsigned defaultHeight)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency,
			 new JPEGBufferedPacketFactory),
    fDefaultWidth(defaultWidth), fDefaultHeight(defaultHeight) {
}

JPEGVideoRTPSource::~JPEGVideoRTPSource() {
}

enum {
	MARKER_SOF0	= 0xc0,		// start-of-frame, baseline scan
	MARKER_SOI	= 0xd8,		// start of image
	MARKER_EOI	= 0xd9,		// end of image
	MARKER_SOS	= 0xda,		// start of scan
	MARKER_DRI	= 0xdd,		// restart interval
	MARKER_DQT	= 0xdb,		// define quantization tables
	MARKER_DHT  = 0xc4,		// huffman tables
	MARKER_APP_FIRST	= 0xe0,
	MARKER_APP_LAST		= 0xef,
	MARKER_COMMENT		= 0xfe,
};

static unsigned char const lum_dc_codelens[] = {
  0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static unsigned char const lum_dc_symbols[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static unsigned char const lum_ac_codelens[] = {
  0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

static unsigned char const lum_ac_symbols[] = {
  0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa,
};

static unsigned char const chm_dc_codelens[] = {
  0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static unsigned char const chm_dc_symbols[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static unsigned char const chm_ac_codelens[] = {
  0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static unsigned char const chm_ac_symbols[] = {
  0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa,
};

static void createHuffmanHeader(unsigned char*& p,
				unsigned char const* codelens,
				int ncodes,
				unsigned char const* symbols,
				int nsymbols,
				int tableNo, int tableClass) {
  *p++ = 0xff; *p++ = MARKER_DHT;
  *p++ = 0;               /* length msb */
  *p++ = 3 + ncodes + nsymbols; /* length lsb */
  *p++ = (tableClass << 4) | tableNo;
  memcpy(p, codelens, ncodes);
  p += ncodes;
  memcpy(p, symbols, nsymbols);
  p += nsymbols;
}

static unsigned computeJPEGHeaderSize(unsigned qtlen, unsigned dri) {
  unsigned qtlen_half = qtlen/2; // in case qtlen is odd; shouldn't happen
  return 495 + qtlen_half*2 + (dri > 0 ? 6 : 0);
}

static void createJPEGHeader(unsigned char* buf, unsigned type,
			     unsigned w, unsigned h,
			     unsigned char const* qtables, unsigned qtlen,
			     unsigned dri) {
  unsigned char *ptr = buf;
  unsigned numQtables = qtlen > 64 ? 2 : 1;

  // MARKER_SOI:
  *ptr++ = 0xFF; *ptr++ = MARKER_SOI;

  // MARKER_APP_FIRST:
  *ptr++ = 0xFF; *ptr++ = MARKER_APP_FIRST;
  *ptr++ = 0x00; *ptr++ = 0x10; // size of chunk
  *ptr++ = 'J'; *ptr++ = 'F'; *ptr++ = 'I'; *ptr++ = 'F'; *ptr++ = 0x00;
  *ptr++ = 0x01; *ptr++ = 0x01; // JFIF format version (1.1)
  *ptr++ = 0x00; // no units
  *ptr++ = 0x00; *ptr++ = 0x01; // Horizontal pixel aspect ratio
  *ptr++ = 0x00; *ptr++ = 0x01; // Vertical pixel aspect ratio
  *ptr++ = 0x00; *ptr++ = 0x00; // no thumbnail

  // MARKER_DRI:
  if (dri > 0) {
    *ptr++ = 0xFF; *ptr++ = MARKER_DRI;
    *ptr++ = 0x00; *ptr++ = 0x04; // size of chunk
    *ptr++ = (BYTE)(dri >> 8); *ptr++ = (BYTE)(dri); // restart interval
  }

  // MARKER_DQT (luma):
  unsigned tableSize = numQtables == 1 ? qtlen : qtlen/2;
  *ptr++ = 0xFF; *ptr++ = MARKER_DQT;
  *ptr++ = 0x00; *ptr++ = tableSize + 3; // size of chunk
  *ptr++ = 0x00; // precision(0), table id(0)
  memcpy(ptr, qtables, tableSize);
  qtables += tableSize;
  ptr += tableSize;

  if (numQtables > 1) {
    unsigned tableSize = qtlen - qtlen/2;
    // MARKER_DQT (chroma):
    *ptr++ = 0xFF; *ptr++ = MARKER_DQT;
    *ptr++ = 0x00; *ptr++ = tableSize + 3; // size of chunk
    *ptr++ = 0x01; // precision(0), table id(1)
    memcpy(ptr, qtables, tableSize);
    qtables += tableSize;
    ptr += tableSize;
  }

  // MARKER_SOF0:
  *ptr++ = 0xFF; *ptr++ = MARKER_SOF0;
  *ptr++ = 0x00; *ptr++ = 0x11; // size of chunk
  *ptr++ = 0x08; // sample precision
  *ptr++ = (BYTE)(h >> 8);
  *ptr++ = (BYTE)(h); // number of lines (must be a multiple of 8)
  *ptr++ = (BYTE)(w >> 8);
  *ptr++ = (BYTE)(w); // number of columns (must be a multiple of 8)
  *ptr++ = 0x03; // number of components
  *ptr++ = 0x01; // id of component
  *ptr++ = type ? 0x22 : 0x21; // sampling ratio (h,v)
  *ptr++ = 0x00; // quant table id
  *ptr++ = 0x02; // id of component
  *ptr++ = 0x11; // sampling ratio (h,v)
  *ptr++ = numQtables == 1 ? 0x00 : 0x01; // quant table id
  *ptr++ = 0x03; // id of component
  *ptr++ = 0x11; // sampling ratio (h,v)
  *ptr++ = 0x01; // quant table id

  createHuffmanHeader(ptr, lum_dc_codelens, sizeof lum_dc_codelens,
		      lum_dc_symbols, sizeof lum_dc_symbols, 0, 0);
  createHuffmanHeader(ptr, lum_ac_codelens, sizeof lum_ac_codelens,
		      lum_ac_symbols, sizeof lum_ac_symbols, 0, 1);
  createHuffmanHeader(ptr, chm_dc_codelens, sizeof chm_dc_codelens,
		      chm_dc_symbols, sizeof chm_dc_symbols, 1, 0);
  createHuffmanHeader(ptr, chm_ac_codelens, sizeof chm_ac_codelens,
		      chm_ac_symbols, sizeof chm_ac_symbols, 1, 1);

  // MARKER_SOS:
  *ptr++ = 0xFF;  *ptr++ = MARKER_SOS;
  *ptr++ = 0x00; *ptr++ = 0x0C; // size of chunk
  *ptr++ = 0x03; // number of components
  *ptr++ = 0x01; // id of component
  *ptr++ = 0x00; // huffman table id (DC, AC)
  *ptr++ = 0x02; // id of component
  *ptr++ = 0x11; // huffman table id (DC, AC)
  *ptr++ = 0x03; // id of component
  *ptr++ = 0x11; // huffman table id (DC, AC)
  *ptr++ = 0x00; // start of spectral
  *ptr++ = 0x3F; // end of spectral
  *ptr++ = 0x00; // successive approximation bit position (high, low)
}

// The default 'luma' and 'chroma' quantizer tables, in zigzag order:
static unsigned char const defaultQuantizers[128] = {
  // luma table:
  16, 11, 12, 14, 12, 10, 16, 14,
  13, 14, 18, 17, 16, 19, 24, 40,
  26, 24, 22, 22, 24, 49, 35, 37,
  29, 40, 58, 51, 61, 60, 57, 51,
  56, 55, 64, 72, 92, 78, 64, 68,
  87, 69, 55, 56, 80, 109, 81, 87,
  95, 98, 103, 104, 103, 62, 77, 113,
  121, 112, 100, 120, 92, 101, 103, 99,
  // chroma table:
  17, 18, 18, 24, 21, 24, 47, 26,
  26, 47, 99, 66, 56, 66, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99
};

static void makeDefaultQtables(unsigned char* resultTables, unsigned Q) {
  int factor = Q;
  int q;

  if (Q < 1) factor = 1;
  else if (Q > 99) factor = 99;

  if (Q < 50) {
    q = 5000 / factor;
  } else {
    q = 200 - factor*2;
  }

  for (int i = 0; i < 128; ++i) {
    int newVal = (defaultQuantizers[i]*q + 50)/100;
    if (newVal < 1) newVal = 1;
    else if (newVal > 255) newVal = 255;
    resultTables[i] = newVal;
  }
}

Boolean JPEGVideoRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  unsigned char* headerStart = packet->data();
  unsigned packetSize = packet->dataSize();

  unsigned char* qtables = NULL;
  unsigned qtlen = 0;
  unsigned dri = 0;

  // There's at least 8-byte video-specific header
  /*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Type-specific |              Fragment Offset                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Type     |       Q       |     Width     |     Height    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  if (packetSize < 8) return False;

  resultSpecialHeaderSize = 8;

  unsigned Offset = (unsigned)((DWORD)headerStart[1] << 16 | (DWORD)headerStart[2] << 8 | (DWORD)headerStart[3]);
  unsigned Type = (unsigned)headerStart[4];
  unsigned type = Type & 1;
  unsigned Q = (unsigned)headerStart[5];
  unsigned width = (unsigned)headerStart[6] * 8;
  unsigned height = (unsigned)headerStart[7] * 8;
  if ((width == 0 || height == 0) && fDefaultWidth != 0 && fDefaultHeight != 0) {
    // Use the default width and height parameters instead:
    width = fDefaultWidth;
    height = fDefaultHeight;
  }
  if (width == 0) width = 256*8; // special case
  if (height == 0) height = 256*8; // special case

  if (Type > 63) {
    // Restart Marker header present
    /*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       Restart Interval        |F|L|       Restart Count       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    if (packetSize < resultSpecialHeaderSize + 4) return False;

    unsigned RestartInterval = (unsigned)((WORD)headerStart[resultSpecialHeaderSize] << 8 | (WORD)headerStart[resultSpecialHeaderSize + 1]);
    dri = RestartInterval;
    resultSpecialHeaderSize += 4;
  }

  if (Offset == 0) {
    if (Q > 127) {
      // Quantization Table header present
/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      MBZ      |   Precision   |             Length            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Quantization Table Data                    |
|                              ...                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
      if (packetSize < resultSpecialHeaderSize + 4) return False;

      unsigned MBZ = (unsigned)headerStart[resultSpecialHeaderSize];
      if (MBZ == 0) {
	// unsigned Precision = (unsigned)headerStart[resultSpecialHeaderSize + 1];
	unsigned Length = (unsigned)((WORD)headerStart[resultSpecialHeaderSize + 2] << 8 | (WORD)headerStart[resultSpecialHeaderSize + 3]);

	//ASSERT(Length == 128);

	resultSpecialHeaderSize += 4;

	if (packetSize < resultSpecialHeaderSize + Length) return False;

	qtlen = Length;
	qtables = &headerStart[resultSpecialHeaderSize];

	resultSpecialHeaderSize += Length;
      }
    }
  }

  // If this is the first (or only) fragment of a JPEG frame, then we need
  // to synthesize a JPEG header, and prepend it to the incoming data.
  // Hack: We can do this because we allowed space for it in
  // our special "JPEGBufferedPacket" subclass.  We also adjust
  // "resultSpecialHeaderSize" to compensate for this, by subtracting
  // the size of the synthesized header.  Note that this will cause
  // "resultSpecialHeaderSize" to become negative, but the code that called
  // us (in "MultiFramedRTPSource") will handle this properly.
  if (Offset == 0) {
    unsigned char newQtables[128];
    if (qtlen == 0) {
      // A quantization table was not present in the RTP JPEG header,
      // so use the default tables, scaled according to the "Q" factor:
      makeDefaultQtables(newQtables, Q);
      qtables = newQtables;
      qtlen = sizeof newQtables;
    }

    unsigned hdrlen = computeJPEGHeaderSize(qtlen, dri);
    resultSpecialHeaderSize -= hdrlen; // goes negative
    headerStart += (int)resultSpecialHeaderSize; // goes backward
    createJPEGHeader(headerStart, type, width, height, qtables, qtlen, dri);
  }

  fCurrentPacketBeginsFrame = (Offset == 0);

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  ((JPEGBufferedPacket*)packet)->completesFrame
   = fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

  return True;
}

char const* JPEGVideoRTPSource::MIMEtype() const {
  return "video/JPEG";
}

////////// JPEGBufferedPacket and JPEGBufferedPacketFactory implementation

void JPEGBufferedPacket::reset() {
  BufferedPacket::reset();

  // Move our "fHead" and "fTail" forward, to allow space for a synthesized
  // JPEG header to precede the RTP data that comes in over the network.
  unsigned offset = MAX_JPEG_HEADER_SIZE;
  if (offset > fPacketSize) offset = fPacketSize; // shouldn't happen
  fHead = fTail = offset;
}

unsigned JPEGBufferedPacket
::nextEnclosedFrameSize(unsigned char*& framePtr, unsigned dataSize) {
  // Normally, the enclosed frame size is just "dataSize".  If, however,
  // the frame does not end with the "EOI" marker, then add this now:
  if (completesFrame && dataSize >= 2 &&
      !(framePtr[dataSize-2] == 0xFF && framePtr[dataSize-1] == MARKER_EOI)) {
    framePtr[dataSize++] = 0xFF;
    framePtr[dataSize++] = MARKER_EOI;
  }
  return dataSize;
}

BufferedPacket* JPEGBufferedPacketFactory
::createNewPacket(MultiFramedRTPSource* /*ourSource*/) {
  return new JPEGBufferedPacket;
}
