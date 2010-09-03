/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "AIFFcodec.h"

#define be2le(x) byte_swap((unsigned char *)&x, sizeof(x))

inline void byte_swap(unsigned char * b, int n)
{
  register int i = 0;
  register int j = n-1;
  while (i<j)
  {
    std::swap(b[i], b[j]);
    i++, j--;
  }
}

#define FORM_ID "FORM"
#define AIFF_ID "AIFF"
#define COMMON_ID "COMM"
#define SOUND_DATA_ID "SSND"

#pragma pack(push, 2)

typedef struct
{
  char chunkID[4];
  long chunkSize;
  char formType[4];
} FORM_CHUNK;

typedef struct {
  char chunkID[4];
  long chunkSize;
} CHUNK;

typedef struct {
  short          numChannels;
  unsigned long  numSampleFrames;
  short          sampleSize;
  unsigned char  sampleRate[10];    // 80 bit sample rate
} COMMON_CHUNK;

typedef struct {
  unsigned long  offset;
  unsigned long  blockSize;
} SOUND_DATA_CHUNK;

#pragma pack(pop)

AIFFCodec::AIFFCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_NumSamples = 0;
  m_iDataStart=0;
  m_iDataLen=0;
  m_Bitrate = 0;
  m_CodecName = "AIFF";
}

AIFFCodec::~AIFFCodec()
{
  DeInit();
}

bool AIFFCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_file.Open(strFile, READ_CACHED))
    return false;

  // read header
  FORM_CHUNK formChunk;
  m_file.Read(&formChunk, sizeof(FORM_CHUNK));
  be2le(formChunk.chunkSize);

  // file valid?
  if (strncmp(formChunk.chunkID, FORM_ID, 4)!=0 && strncmp(formChunk.formType, AIFF_ID, 4)!=0)
    return false;

  long offset=0;
  offset += sizeof(FORM_CHUNK);
  offset -= sizeof(CHUNK);

  // parse chunks
  do
  {
    CHUNK chunk;

    // always seeking to the start of a chunk
    m_file.Seek(offset + sizeof(CHUNK), SEEK_SET);
    m_file.Read(&chunk, sizeof(CHUNK));
    be2le(chunk.chunkSize);

    if (!strncmp(chunk.chunkID, COMMON_ID, 4))
    { // common chunk
      // read in our data
      COMMON_CHUNK common;
      memset(&common, 0, sizeof(COMMON_CHUNK));
      m_file.Read(&common, sizeof(COMMON_CHUNK));
      be2le(common.numChannels);
      be2le(common.sampleSize);
      be2le(common.numSampleFrames);

      //  Get file info
      m_Channels = common.numChannels;
      m_BitsPerSample = (common.sampleSize + 7) & (~7);  // rounded to 8 bits
      switch(m_BitsPerSample)
      {
        case  8: m_DataFormat = AE_FMT_U8;    break;
        case 16: m_DataFormat = AE_FMT_S16NE; break;
      }
      m_NumSamples = common.numSampleFrames;
      m_SampleRate = ConvertSampleRate(common.sampleRate);
    }
    else if (!strncmp(chunk.chunkID, SOUND_DATA_ID, 4))
    { // data chunk
      SOUND_DATA_CHUNK dataChunk;
      memset(&dataChunk, 0, sizeof(SOUND_DATA_CHUNK));
      m_file.Read(&dataChunk, sizeof(SOUND_DATA_CHUNK));

      m_iDataStart=(long)m_file.GetPosition() + dataChunk.offset;
      m_iDataLen=chunk.chunkSize - sizeof(SOUND_DATA_CHUNK) - dataChunk.offset;
    }
    else
    { // other chunk - unused, just skip
      m_file.Seek(chunk.chunkSize, SEEK_CUR);
    }

    offset += (chunk.chunkSize+sizeof(CHUNK));

  } while (offset+(int)sizeof(CHUNK) < formChunk.chunkSize);

  m_TotalTime = (int)((float)m_NumSamples/m_SampleRate*1000);
  m_Bitrate = (int)(((float)m_iDataLen * 8) / ((float)m_TotalTime / 1000));

  // we only support 8 and 16 bit samples
  if (m_SampleRate==0 || m_Channels==0 || (m_BitsPerSample!=8 && m_BitsPerSample!=16) || m_TotalTime==0 || m_iDataStart==0 || m_iDataLen==0)
    return false;

  //  Seek to the start of the data chunk
  m_file.Seek(m_iDataStart);

  return true;
}

void AIFFCodec::DeInit()
{
  m_file.Close();
}

__int64 AIFFCodec::Seek(__int64 iSeekTime)
{
  //  Calculate size of a single sample of the file
  int iSampleSize=m_SampleRate*m_Channels*(m_BitsPerSample/8);

  //  Seek to the position in the file
  m_file.Seek(m_iDataStart+((iSeekTime/1000)*iSampleSize));

  return iSeekTime;
}

int AIFFCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iPos=(int)m_file.GetPosition();
  if (iPos >= m_iDataStart+m_iDataLen)
    return READ_EOF;

  int iAmountRead=m_file.Read(pBuffer, size);
  if (iAmountRead>0)
  {
    // convert to little endian
    if (m_BitsPerSample == 16)
    {
      for (int i = 0; i < iAmountRead; i+=2)
      {
        BYTE temp = pBuffer[i+1];
        pBuffer[i+1] = pBuffer[i];
        pBuffer[i] = temp;
      }
    }
    *actualsize=iAmountRead;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool AIFFCodec::CanInit()
{
  return true;
}

int AIFFCodec::ConvertSampleRate(unsigned char *rate)
{
  if ( memcmp(rate, "\x40\x0E\xAC\x44", 4) == 0 )
    return 44100;
  else if ( memcmp(rate, "\x40\x0E\xBB\x80", 4) == 0 )
    return 48000;
  else if ( memcmp(rate, "\x40\x0D\xFA", 3) == 0 )
    return 32000;
  else if ( memcmp(rate, "\x40\x0D\xBB\x80", 4) == 0 )
    return 24000;
  else if ( memcmp(rate, "\x40\x0D\xAC\x44", 4) == 0 )
    return 22050;
  else if ( memcmp(rate, "\x40\x0C\xFA", 3) == 0 )
    return 16000;
  else if ( memcmp(rate, "\x40\x0C\xAC\x44", 4) == 0 )
    return 11025;
  else if ( memcmp(rate, "\x40\x0B\xFA", 3) == 0 )
    return 8000;
  return 0;
}
