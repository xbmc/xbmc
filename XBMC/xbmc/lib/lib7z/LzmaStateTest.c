/* 
LzmaStateTest.c
Test application for LZMA Decoder (State version)

This file written and distributed to public domain by Igor Pavlov.
This file is part of LZMA SDK 4.26 (2005-08-02)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LzmaStateDecode.h"

const char *kCantReadMessage = "Can not read input file";
const char *kCantWriteMessage = "Can not write output file";
const char *kCantAllocateMessage = "Can not allocate memory";

#define kInBufferSize (1 << 15)
#define kOutBufferSize (1 << 15)

unsigned char g_InBuffer[kInBufferSize];
unsigned char g_OutBuffer[kOutBufferSize];

size_t MyReadFile(FILE *file, void *data, size_t size)
  { return fread(data, 1, size, file); }

int MyReadFileAndCheck(FILE *file, void *data, size_t size)
  { return (MyReadFile(file, data, size) == size); }

int PrintError(char *buffer, const char *message)
{
  sprintf(buffer + strlen(buffer), "\nError: ");
  sprintf(buffer + strlen(buffer), message);
  return 1;
}

int main3(FILE *inFile, FILE *outFile, char *rs)
{
  /* We use two 32-bit integers to construct 64-bit integer for file size.
     You can remove outSizeHigh, if you don't need >= 4GB supporting,
     or you can use UInt64 outSize, if your compiler supports 64-bit integers*/
  UInt32 outSize = 0;
  UInt32 outSizeHigh = 0; 
  
  int waitEOS = 1; 
  /* waitEOS = 1, if there is no uncompressed size in headers, 
   so decoder will wait EOS (End of Stream Marker) in compressed stream */

  int i;
  int res = 0;
  CLzmaDecoderState state;  /* it's about 140 bytes structure, if int is 32-bit */
  unsigned char properties[LZMA_PROPERTIES_SIZE];
  SizeT inAvail = 0;
  unsigned char *inBuffer = 0;

  if (sizeof(UInt32) < 4)
    return PrintError(rs, "LZMA decoder needs correct UInt32");

  /* Read LZMA properties for compressed stream */

  if (!MyReadFileAndCheck(inFile, properties, sizeof(properties)))
    return PrintError(rs, kCantReadMessage);

  /* Read uncompressed size */
  
  for (i = 0; i < 8; i++)
  {
    unsigned char b;
    if (!MyReadFileAndCheck(inFile, &b, 1))
      return PrintError(rs, kCantReadMessage);
    if (b != 0xFF)
      waitEOS = 0;
    if (i < 4)
      outSize += (UInt32)(b) << (i * 8);
    else
      outSizeHigh += (UInt32)(b) << ((i - 4) * 8);
  }

  /* Decode LZMA properties and allocate memory */
  
  if (LzmaDecodeProperties(&state.Properties, properties, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK)
    return PrintError(rs, "Incorrect stream properties");
  state.Probs = (CProb *)malloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
  if (state.Probs == 0)
    return PrintError(rs, kCantAllocateMessage);
  
  if (state.Properties.DictionarySize == 0)
    state.Dictionary = 0;
  else
  {
    state.Dictionary = (unsigned char *)malloc(state.Properties.DictionarySize);
    if (state.Dictionary == 0)
    {
      free(state.Probs);
      return PrintError(rs, kCantAllocateMessage);
    }
  }
  
  /* Decompress */
  
  LzmaDecoderInit(&state);
  
  do
  {
    SizeT inProcessed, outProcessed;
    int finishDecoding;
    UInt32 outAvail = kOutBufferSize;
    if (!waitEOS && outSizeHigh == 0 && outAvail > outSize)
      outAvail = outSize;
    if (inAvail == 0)
    {
      inAvail = (SizeT)MyReadFile(inFile, g_InBuffer, kInBufferSize);
      inBuffer = g_InBuffer;
    }
    finishDecoding = (inAvail == 0);
    res = LzmaDecode(&state,
        inBuffer, inAvail, &inProcessed,
        g_OutBuffer, outAvail, &outProcessed,
        finishDecoding);
    if (res != 0)
    {
      sprintf(rs + strlen(rs), "\nDecoding error = %d\n", res);
      res = 1;
      break;
    }
    inAvail -= inProcessed;
    inBuffer += inProcessed;
    
    if (outFile != 0)  
      if (fwrite(g_OutBuffer, 1, outProcessed, outFile) != outProcessed)
      {
        PrintError(rs, kCantWriteMessage);
        res = 1;
        break;
      }
      
    if (outSize < outProcessed)
      outSizeHigh--;
    outSize -= (UInt32)outProcessed;
    outSize &= 0xFFFFFFFF;

    if (outProcessed == 0 && finishDecoding)
    {
      if (!waitEOS && (outSize != 0 || outSizeHigh != 0))
        res = 1;
      break;
    }
  }
  while ((outSize != 0 && outSizeHigh == 0) || outSizeHigh != 0  || waitEOS);

  free(state.Dictionary);
  free(state.Probs);
  return res;
}

int main2(int numArgs, const char *args[], char *rs)
{
  FILE *inFile = 0;
  FILE *outFile = 0;
  int res;

  sprintf(rs + strlen(rs), "\nLZMA Decoder 4.26 Copyright (c) 1999-2005 Igor Pavlov  2005-08-02\n");
  if (numArgs < 2 || numArgs > 3)
  {
    sprintf(rs + strlen(rs), "\nUsage:  lzmadec file.lzma [outFile]\n");
    return 1;
  }

  inFile = fopen(args[1], "rb");
  if (inFile == 0)
    return PrintError(rs, "Can not open input file");

  if (numArgs > 2)
  {
    outFile = fopen(args[2], "wb+");
    if (outFile == 0)
      return PrintError(rs, "Can not open output file");
  }

  res = main3(inFile, outFile, rs);

  if (outFile != 0)
    fclose(outFile);
  fclose(inFile);
  return res;
}

int main(int numArgs, const char *args[])
{
  char rs[800] = { 0 };
  int res = main2(numArgs, args, rs);
  printf(rs);
  return res;
}
