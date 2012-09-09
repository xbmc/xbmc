/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CDDAcodec.h"
#if !defined(TARGET_DARWIN_IOS)
#include <cdio/sector.h>
#else
typedef int32_t lsn_t;
#define CDIO_CD_FRAMESIZE_RAW   2352
#define CDIO_CD_FRAMES_PER_SEC   75
#endif
#include "cores/AudioEngine/Utils/AEUtil.h"

#define SECTOR_COUNT 55 // max. sectors that can be read at once
#define MAX_BUFFER_SIZE 2*SECTOR_COUNT*CDIO_CD_FRAMESIZE_RAW

CDDACodec::CDDACodec()
{
  m_SampleRate = 44100;
  m_Channels = 2;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "CDDA";

  m_BufferSize=0;
  m_Buffer = new BYTE[MAX_BUFFER_SIZE];
  m_BufferPos = 0;
}

CDDACodec::~CDDACodec()
{
  DeInit();

  if (m_Buffer)
  {
    delete[] m_Buffer;
    m_Buffer = NULL;
  }
}

bool CDDACodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_file.Open(strFile, READ_CACHED))
    return false;

  //  Calculate total time of the track
  m_TotalTime=(m_file.GetLength()/CDIO_CD_FRAMESIZE_RAW)/CDIO_CD_FRAMES_PER_SEC;
  if (m_TotalTime > 0)
    m_Bitrate = (int)((m_file.GetLength() * 8) / m_TotalTime);
  else
    m_Bitrate = 0;
  m_TotalTime*=1000; // ms
  return true;
}

void CDDACodec::DeInit()
{
  m_file.Close();
}

int64_t CDDACodec::Seek(int64_t iSeekTime)
{
  //  Calculate the next full second...
  int iSeekTimeFullSec=(int)(iSeekTime+(1000-(iSeekTime%1000)))/1000;

  //  ...and the logical sector on the cd...
  lsn_t lsnSeek=iSeekTimeFullSec*CDIO_CD_FRAMES_PER_SEC;

  //  ... then seek to its position...
  int iNewOffset=(int)m_file.Seek(lsnSeek*CDIO_CD_FRAMESIZE_RAW, SEEK_SET);
  m_BufferSize=m_BufferPos=0;

  // ... and look if we really got there.
  int iNewSeekTime=(iNewOffset/CDIO_CD_FRAMESIZE_RAW)/CDIO_CD_FRAMES_PER_SEC;
  return iNewSeekTime*(int64_t)1000; // ms
}

int CDDACodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;

  bool bEof=false;
  //  Reached end of track?
  if (m_file.GetLength()==m_file.GetPosition())
    bEof=true;

  //  Do we have to refill our audio buffer?
  if (m_BufferSize-m_BufferPos<MAX_BUFFER_SIZE/2 && !bEof)
  {
    //  Move the remaining audio data to the beginning of the buffer
    memmove(m_Buffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    m_BufferSize=m_BufferSize-m_BufferPos;
    m_BufferPos = 0;

    //  Fill our buffer with a chunk of audio data
    int iAmountRead=m_file.Read(m_Buffer+m_BufferSize, MAX_BUFFER_SIZE/2);
    if (iAmountRead<=0)
      return READ_ERROR;

    m_BufferSize+=iAmountRead;
  }

  //  Our buffer is empty and no data left to read from the cd
  if (m_BufferSize-m_BufferPos==0 && bEof)
    return READ_EOF;

  //  Try to give the player the amount of audio data he wants
  if (m_BufferSize-m_BufferPos>=size)
  { //  we have enough data in our buffer
    memcpy(pBuffer, m_Buffer + m_BufferPos, size);
    m_BufferPos+=size;
    *actualsize=size;
  }
  else
  { //  Only a smaller amount of data left as the player wants
    memcpy(pBuffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    *actualsize=m_BufferSize-m_BufferPos;
    m_BufferPos+=m_BufferSize-m_BufferPos;
  }

  return READ_SUCCESS;
}

bool CDDACodec::CanInit()
{
  return true;
}

CAEChannelInfo CDDACodec::GetChannelInfo()
{
  static enum AEChannel map[2][3] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR  , AE_CH_NULL}
  };

  if (m_Channels > 2)
    return CAEUtil::GuessChLayout(m_Channels);

  return CAEChannelInfo(map[m_Channels - 1]);
}
