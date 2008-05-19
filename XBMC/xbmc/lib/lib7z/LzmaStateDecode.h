/* 
  LzmaStateDecode.h
  LZMA Decoder interface (State version)

  LZMA SDK 4.40 Copyright (c) 1999-2006 Igor Pavlov (2006-05-01)
  http://www.7-zip.org/

  LZMA SDK is licensed under two licenses:
  1) GNU Lesser General Public License (GNU LGPL)
  2) Common Public License (CPL)
  It means that you can select one of these two licenses and 
  follow rules of that license.

  SPECIAL EXCEPTION:
  Igor Pavlov, as the author of this code, expressly permits you to 
  statically or dynamically link your code (or bind by name) to the 
  interfaces of this file without subjecting your linked code to the 
  terms of the CPL or GNU LGPL. Any modifications or additions 
  to this file, however, are subject to the LGPL or CPL terms.
*/

#ifndef __LZMASTATEDECODE_H
#define __LZMASTATEDECODE_H

#include "LzmaTypes.h"

/* #define _LZMA_PROB32 */
/* It can increase speed on some 32-bit CPUs, 
   but memory usage will be doubled in that case */

#ifdef _LZMA_PROB32
#define CProb UInt32
#else
#define CProb UInt16
#endif

#define LZMA_RESULT_OK 0
#define LZMA_RESULT_DATA_ERROR 1

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#define LZMA_PROPERTIES_SIZE 5

typedef struct _CLzmaProperties
{
  int lc;
  int lp;
  int pb;
  UInt32 DictionarySize;
}CLzmaProperties;

int LzmaDecodeProperties(CLzmaProperties *propsRes, const unsigned char *propsData, int size);

#define LzmaGetNumProbs(lzmaProps) (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((lzmaProps)->lc + (lzmaProps)->lp)))

#define kLzmaInBufferSize 64   /* don't change it. it must be larger than kRequiredInBufferSize */

#define kLzmaNeedInitId (-2)

typedef struct _CLzmaDecoderState
{
  CLzmaProperties Properties;
  CProb *Probs;
  unsigned char *Dictionary;

  unsigned char Buffer[kLzmaInBufferSize];
  int BufferSize;

  UInt32 Range;
  UInt32 Code;
  UInt32 DictionaryPos;
  UInt32 GlobalPos;
  UInt32 DistanceLimit;
  UInt32 Reps[4];
  int State;
  int RemainLen;  /* -2: decoder needs internal initialization
                     -1: stream was finished, 
                      0: ok
                    > 0: need to write RemainLen bytes as match Reps[0],
                  */
  unsigned char TempDictionary[4];  /* it's required when DictionarySize = 0 */
} CLzmaDecoderState;

#define LzmaDecoderInit(vs) { (vs)->RemainLen = kLzmaNeedInitId; (vs)->BufferSize = 0; }

/* LzmaDecode: decoding from input stream to output stream.
  If finishDecoding != 0, then there are no more bytes in input stream
  after inStream[inSize - 1]. */

int LzmaDecode(CLzmaDecoderState *vs,
    const unsigned char *inStream, SizeT inSize,  SizeT *inSizeProcessed,
    unsigned char *outStream, SizeT outSize, SizeT *outSizeProcessed,
    int finishDecoding);

#endif
