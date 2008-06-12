/*
 * Copyright (c) 2003, Eric F.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

typedef unsigned int uint32;

uint32 byteSwap(uint32 i);
void fixEntryPoint(FILE *fp, uint32 *address);
void fixKernelThunkAddress(FILE *fp, uint32 *address);

int main(int argc, char **argv)
{
 FILE *in;
 uint32 entryPoint, kernelThunk;

 if(argc < 2)
  {
   printf("Usage: xbepatch debug.xbe\n");
   return 1;
  }

 in = fopen(argv[1],"r+b");
 if(in == NULL)
  {
   printf("\nError: Opening File %s for reading.\n",argv[1]);
   return 1;
  }

 fseek(in,0x128,SEEK_SET);
 fread(&entryPoint,4,1,in);

 entryPoint ^= byteSwap(0x94859D4B); /* debug key (little endian)*/
 entryPoint ^= byteSwap(0xA8FC57AB); /* retail key (little endian)*/

 fseek(in,0x128,SEEK_SET);
 fwrite(&entryPoint,4,1,in);

 fseek(in,0x158,SEEK_SET);
 fread(&kernelThunk,4,1,in);

 kernelThunk ^= byteSwap(0xEFB1F152); /* debug key (little endian)*/
 kernelThunk ^= byteSwap(0x5B6D40B6); /* retail key (little endian)*/

 fseek(in,0x158,SEEK_SET);
 fwrite(&kernelThunk,4,1,in);

 fclose(in);
 return 0;
}

uint32 byteSwap(uint32 l)
{

#ifdef BIG_ENDIAN 
 unsigned char buf[4];
 unsigned char temp;
 int i;

 memcpy(buf,&l,4);

 for(i=0;i<2;i++)
  {
   temp = buf[i];
   buf[i] = buf[4-i-1];
   buf[4-i-1] = temp;
  }

 memcpy(&l,buf,4);
#endif /* BIG_ENDIAN */

 return l;
}

void fixEntryPoint(FILE *fp, unsigned int *address)
{
 fseek(fp,0x128,SEEK_SET);

 fread(address,4,1,fp);

 *address ^= byteSwap(0x94859D4B); /* debug key (little endian)*/

 *address ^= byteSwap(0xA8FC57AB); /* retail key (little endian)*/

 return; 
}

void fixKernelThunkAddress(FILE *fp, unsigned int *address)
{
 fseek(fp,0x158,SEEK_SET);

 fread(address,4,1,fp);

 *address ^= byteSwap(0xEFB1F152); /* debug key (little endian)*/

 *address ^= byteSwap(0x5B6D40B6); /* retail key (little endian)*/

 return; 
}

