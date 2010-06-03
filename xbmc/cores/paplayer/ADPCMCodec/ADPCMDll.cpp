/*
 *      Copyright (C) 2008-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _LINUX
#define __declspec(x)
#endif

extern "C" 
{
#include "mywav.h"
#include "uXboxAdpcmDecoder.h"
#include <stdlib.h>

#define GETLENGTH(x,SAMPLERATE,NCH)    (((x * 10) / ((SAMPLERATE / 100) * NCH * (16 >> 3)) / XBOX_ADPCM_SRCSIZE) * XBOX_ADPCM_DSTSIZE)

  struct ADPCMInfo
  {
    FILE* f;
    mywav_fmtchunk  fmt;
    unsigned int length;
    int data_offset;
    char* szInputBuffer;
    char* szBuf;
    char* szStartOfBuf;
    int bufLen;
  };

  int getwavinfo(ADPCMInfo* info) {
    int     wavsize;

    wavsize = mywav_data(info->f, &info->fmt);
    if(wavsize >= 0) {
        if(info->fmt.wFormatTag != 0x0069) {
            fseek(info->f,0,SEEK_SET);
            return(-1);
        }
        info->data_offset = ftell(info->f);
    } else {
        fseek(info->f,0,SEEK_END);
        wavsize     = ftell(info->f);
        fseek(info->f,0,SEEK_SET);
    }

    info->length = GETLENGTH(wavsize,info->fmt.dwSamplesPerSec,info->fmt.wChannels);
    return(wavsize);
}


  long __declspec(dllexport) DLL_LoadXWAV(const char* szFileName)
  { 
    ADPCMInfo* info = (ADPCMInfo*)malloc(sizeof(ADPCMInfo));
    info->f = fopen(szFileName,"rb");
    if (!info->f)
    {
      free(info);
      return NULL;
    }

    int iResult = getwavinfo(info);
    if (iResult == -1)
    {
      fclose(info->f);
      free(info);
      return NULL;
    }

    info->szBuf = (char*)malloc(XBOX_ADPCM_DSTSIZE*info->fmt.wChannels*4);
    info->szInputBuffer = (char*)malloc(XBOX_ADPCM_SRCSIZE*info->fmt.wChannels*4);
    info->szStartOfBuf = info->szBuf+XBOX_ADPCM_DSTSIZE*info->fmt.wChannels*4;
    info->bufLen = XBOX_ADPCM_DSTSIZE*info->fmt.wChannels*4;
    return (long)info;
  }

  void __declspec(dllexport) DLL_FreeXWAV(int info)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    fclose(pInfo->f);
    free(pInfo);
  }
    
  int __declspec(dllexport) DLL_Seek(int info, int pos)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    int offs = pInfo->data_offset + ((((pos/ 1000) * pInfo->fmt.dwSamplesPerSec) / XBOX_ADPCM_DSTSIZE) * XBOX_ADPCM_SRCSIZE * pInfo->fmt.wChannels * (16 >> 3));
    
    fseek(pInfo->f,offs,SEEK_SET);
    // invalidate buffer
    pInfo->szStartOfBuf = pInfo->szBuf+XBOX_ADPCM_DSTSIZE*pInfo->fmt.wChannels*4;
    return pos;
  }

  long __declspec(dllexport) DLL_FillBuffer(int info, char* buffer, int size)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    int iCurrSize = size;
    while (iCurrSize > 0)
    {
      if (pInfo->szStartOfBuf >= pInfo->szBuf+pInfo->bufLen)
      {          
        // Read data into input buffer
        int read = fread(pInfo->szInputBuffer,XBOX_ADPCM_SRCSIZE*pInfo->fmt.wChannels,4,pInfo->f);
        if (!read)
          break;

        TXboxAdpcmDecoder_Decode_Memory((uint8_t*)pInfo->szInputBuffer, read*XBOX_ADPCM_SRCSIZE*pInfo->fmt.wChannels, (uint8_t*)pInfo->szBuf, pInfo->fmt.wChannels);
        pInfo->szStartOfBuf = pInfo->szBuf;
      }
      int iCopy=0;
      if (iCurrSize > pInfo->szBuf+pInfo->bufLen-pInfo->szStartOfBuf)
        iCopy = pInfo->szBuf+pInfo->bufLen-pInfo->szStartOfBuf;
      else
        iCopy = iCurrSize;

      memcpy(buffer,pInfo->szStartOfBuf,iCopy);
      buffer += iCopy;
      iCurrSize -= iCopy;
      pInfo->szStartOfBuf += iCopy;
    }

    return size-iCurrSize;
  }

  int __declspec(dllexport) DLL_GetPlaybackRate(int info)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    return pInfo->fmt.dwSamplesPerSec;
  }

  int __declspec(dllexport) DLL_GetNumberOfChannels(int info)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    return pInfo->fmt.wChannels;
  }

  int __declspec(dllexport) DLL_GetSampleSize(int info)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    return pInfo->fmt.wBitsPerSample;
  }

  int __declspec(dllexport) DLL_GetLength(int info)
  {
    ADPCMInfo* pInfo = (ADPCMInfo*)info;
    return pInfo->length;
  }
}

