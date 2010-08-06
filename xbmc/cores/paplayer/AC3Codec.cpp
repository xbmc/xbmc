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

#include "system.h"
#include "AC3Codec.h"
#include "utils/log.h"

#ifdef USE_LIBA52_DECODER

static inline int16_t convert(int32_t i)
{
#ifdef LIBA52_FIXED
  i >>= 15;
#else
  i -= 0x43c00000;
#endif
  return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

/**
 * \brief Function to convert the "planar" float format used by liba52
 * into the interleaved int16 format used by us.
 * \param in the input buffer containing the planar samples.
 * \param out the output buffer where the interleaved result is stored.
 */
static int resample_int16(sample_t * in, int16_t *out, int32_t channel_map)
{
  unsigned long i;
  int16_t *p = out;
  for (i = 0; i != 256; i++)
  {
    unsigned long map = channel_map;
    do
	{
      unsigned long ch = map & 15;
      if (ch == 15)
        *p = 0;
      else
        *p = convert( ((int32_t*)in)[i + ((ch-1)<<8)] );
      p++;
    } while ((map >>= 4));
  }
  return (int16_t*) p - out;
}

AC3Codec::AC3Codec()
{
  m_CodecName         = "AC3";
  m_IsInitialized     = false;
  m_pState            = NULL;
  m_iSourceChannels   = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate    = 0;
  m_decodedDataSize   = 0;
  m_pInputBuffer      = NULL;
  m_eof               = false;
  m_readBuffer        = NULL;
  m_decodedData       = NULL;
  m_readBufferPos     = 0;
  m_iOutputChannels   = 0;
  m_iOutputMapping    = 0;
}

AC3Codec::~AC3Codec()
{
  DeInit();
}

void AC3Codec::DeInit()
{
  m_IsInitialized = false;
  if (m_pState) m_dll.a52_free(m_pState);
  m_pState = NULL;

  CloseFile();
  m_readBufferPos = 0;
  if (m_readBuffer)  SAFE_DELETE_ARRAY(m_readBuffer);
  if (m_decodedData) SAFE_DELETE_ARRAY(m_decodedData);
}

__int64 AC3Codec::Seek(__int64 iSeekTime)
{
  //  Calculate size of a single sample of the file
  int iSampleSize = m_SampleRate * 2 * (m_BitsPerSample / 8);
  //  Seek to the position in the file
  __int64 newPos = m_file.Seek(m_iDataStart + ((iSeekTime / 1000) * iSampleSize));
  m_readBufferPos = 0;
  m_iFrameSize    = 0;

  return ((newPos - m_iDataStart) * 1000) / iSampleSize;
}

bool AC3Codec::CanInit()
{
  return m_dll.CanLoad();
}


void AC3Codec::PrepairBuffers()
{
  m_decodingBufferSize = 131072;
  m_readingBufferSize  = 98304;
  m_decodedData        = new BYTE[m_decodingBufferSize];
  m_readBuffer         = new BYTE[m_readingBufferSize];
  m_readBufferPos      = m_file.Read(m_readBuffer, m_readingBufferSize); // big read, much crap sometimes before data starts....
}

bool AC3Codec::InitFile(const CStdString &strFile, unsigned int filecache)
{
  //  Open the file to play
  return m_file.Open(strFile, READ_CACHED);
}

bool AC3Codec::CalculateTotalTime()
{
  //liba52 gives half the actual bitrate sometime causing the total time to double
  //don't know why this is, vlc has the same problem
  m_TotalTime = (((m_file.GetLength() - m_iDataStart) * 8) / m_Bitrate) * 1000;
  return m_TotalTime > 0;
}

bool AC3Codec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (m_IsInitialized) return true;
  if (!InitFile(strFile, filecache))
  {
    CLog::Log(LOGERROR, "AC3Codec: Can't open %s", strFile.c_str());
    return false;
  }
  if (!m_dll.Load())
    return false;

  SetDefault();

  m_pState = m_dll.a52_init(0);
  if (!m_pState)
    return false;

  m_eof = false;

  PrepairBuffers();

  int istart = 0;
  do
  {
    m_iDataStart = -1;
    Decode(m_readBuffer + istart, m_readBufferPos - istart);
    //decoding error, could still be a valid ac3-stream, try again at next byte
    istart += m_iDataStart + 1;
  } while (m_DecoderError && (m_decodedDataSize == 0) && (m_readBufferPos - istart > 0) );
  if (m_DecoderError || m_decodedDataSize == 0)
  {
    return false;
  }
  m_Channels      = m_iOutputChannels;
  m_SampleRate    = m_iSourceSampleRate;
  m_BitsPerSample = 16;
  m_Bitrate       = m_iSourceBitrate;
  if (m_Channels == 0 || m_SampleRate == 0)
  {
    return false;
  }

  if (!CalculateTotalTime())
    return false;

  //m_file.StartBuffering();
  m_IsInitialized = true;
  return true;
}

void AC3Codec::CloseFile()
{
  m_file.Close();
}

void AC3Codec::SetupChannels(unsigned flags)
{
  m_iFlags = flags;
  // setup channel map for how to translate to linear format
  // standard windows format
  if(m_iFlags & A52_LFE)
  {
    switch (m_iFlags&~A52_LFE)
	{
      case A52_MONO   : m_iOutputMapping = 0x12ffff; break;
      case A52_CHANNEL:
      case A52_STEREO :
      case A52_DOLBY  : m_iOutputMapping = 0x1fff32; break;
      case A52_2F1R   : m_iOutputMapping = 0x1f5542; break;
      case A52_2F2R   : m_iOutputMapping = 0x1f5432; break;
      case A52_3F     : m_iOutputMapping = 0x13ff42; break;
      case A52_3F2R   : m_iOutputMapping = 0x136542; break;
    }
  }
  else
  {
    switch (m_iFlags)
	{
      case A52_MONO   : m_iOutputMapping =     0x1; break;
      case A52_CHANNEL:
      case A52_STEREO :
      case A52_DOLBY  : m_iOutputMapping =    0x21; break;
      case A52_2F1R   : m_iOutputMapping =  0x2231; break;
      case A52_2F2R   : m_iOutputMapping =  0x4321; break;
      case A52_3F     : m_iOutputMapping = 0x2ff31; break;
      case A52_3F2R   : m_iOutputMapping = 0x25431; break;
    }
  }

  int channels = 0;
  unsigned int m = m_iOutputMapping<<4;
  while(m>>=4) channels++;

  // xbox can't handle these
  if(channels == 5 || channels == 3)
    channels = 6;

  if(!m_iOutputChannels)
    m_iOutputChannels = channels;
  else if(m_iOutputChannels != channels)
    CLog::Log(LOGWARNING, "%s - Number of channels changed in stream from %d to %d, data might be truncated", __FUNCTION__, m_iOutputChannels, channels);

  // make sure map contains enough channels
  for(int i=0;i<m_iOutputChannels;i++)
  {
    if((m_iOutputMapping & (0xf<<(i*4))) == 0)
      m_iOutputMapping |= 0xf<<(i*4);
  }
  // and nothing more
  m_iOutputMapping &= ~(0xffffffff<<(m_iOutputChannels*4));

  static const int ac3Channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
  m_iSourceChannels = ac3Channels[m_iFlags & 7];
  if (m_iFlags & A52_LFE) m_iSourceChannels++;
}

int AC3Codec::ReadInput()
{
  int inputBufferToRead = m_readingBufferSize - m_readBufferPos;
  if ( inputBufferToRead && !m_eof )
  {
    int fileLeft=(int)(m_file.GetLength() - m_file.GetPosition());
    if (inputBufferToRead >  fileLeft ) inputBufferToRead = fileLeft;

    DWORD dwBytesRead = m_file.Read(m_readBuffer + m_readBufferPos, inputBufferToRead);
    if (!dwBytesRead)
    {
      CLog::Log(LOGERROR, "AC3Codec: Error reading file");
      return -1;
    }
    m_readBufferPos += dwBytesRead;
    if ( m_file.GetLength() == m_file.GetPosition() )
      m_eof = true;
    return dwBytesRead;
  }
  return 0;
}

int AC3Codec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = 0;
  // First read in data from our file
  if (ReadInput() == -1)
  {
    return READ_ERROR;
  }
  // Decode data if we have some to decode
  if ( size )
  {
    if ((m_readBufferPos > 0) && (m_decodedDataSize < m_decodingBufferSize - 18432))
    {
      // Now decode data into the vacant frame buffer
      int result = Decode(m_readBuffer, m_readBufferPos);
      if (result == -1)
        return READ_ERROR;
      memmove(m_readBuffer, m_readBuffer + result, m_readBufferPos - result);
      m_readBufferPos -= result;
    }
  }
  // check whether we can move data out of our output buffer
  int amounttomove = m_decodedDataSize;
  if (amounttomove > size) amounttomove = size;
  if (amounttomove)
  {
    memcpy(pBuffer, m_decodedData, amounttomove);
    m_decodedDataSize -= amounttomove;
    memmove(m_decodedData, m_decodedData + amounttomove, m_decodedDataSize);
    *actualsize = amounttomove;
  }
  if (m_eof)
    return READ_EOF;
  return READ_SUCCESS;
}

#define HEADER_SIZEAC3 7
int AC3Codec::Decode(BYTE* pData, int iSize)
{
  sample_t bias = 384;
  m_DecoderError = false;

  int iLen = 0;
  BYTE* pOldDataPointer = pData;
  while (iSize > HEADER_SIZEAC3)
  {
    if (m_iFrameSize == 0)
    {
      // no header seen : find one. We need at least 7 + 1 bytes to parse it
      int i = 0;
      while (i < (iSize - HEADER_SIZEAC3))
      {
        // it's possible that m_inputBuffer already contains 6 bits from our previous run
        // so use m_pInputBuffer to copy the rest of the data. We must rest it after a52_syncinfo though!!
        for (int u = 0; u <= HEADER_SIZEAC3; u++) m_pInputBuffer[u] = pData[u];

        int flags;
#if (defined USE_EXTERNAL_LIBA52)
        iLen = m_dll.a52_syncinfo(m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
#else
        iLen = m_dll.a52_syncinfo(m_pState, m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
#endif
        if (iLen > 0)
        {
          if(flags != m_iFlags)
            SetupChannels(flags);

          m_iFrameSize = iLen;
          //start of data in file
          if (m_iDataStart == -1)
            m_iDataStart = (pData - pOldDataPointer);

          pData          += HEADER_SIZEAC3;
          m_pInputBuffer += HEADER_SIZEAC3;
          iSize          -= HEADER_SIZEAC3;
          break;
        }

        // reset the buffer pointer if needed
        if ((m_pInputBuffer - m_inputBuffer) > 0) m_pInputBuffer = m_inputBuffer;

        // no sync found, shift one byte
        i++;
        pData++;
        iSize--;
      }
      if (m_iFrameSize == 0 && iSize < HEADER_SIZEAC3)
      {
        // we are at the end of our stream and don't have enough data for our header anymore.
        // copy it to our buffer for later use;
        for (int i = 0; i < iSize; i++) m_pInputBuffer[i] = pData[i];

        m_pInputBuffer += iSize;
        pData          += iSize;
        iSize           = 0;
        break;
      }
    }
    else if (m_pInputBuffer - m_inputBuffer < m_iFrameSize)
    {
      // we are working on a frame that is m_iFrameSize big, but we don't have all data yet
      // just copy more data to it
      iLen = m_iFrameSize - (m_pInputBuffer - m_inputBuffer);
      if (iSize < iLen) iLen = iSize;
      memcpy(m_pInputBuffer, pData, iLen);

      m_pInputBuffer += iLen;
      pData          += iLen;
      iSize          -= iLen;
    }
    else
    {
      // we have a frame to decode
      float level = 1.0f;
      int iFlags = m_iFlags;

      /* adjust level should always be set, to keep samples in proper range */
      /* after any downmixing has been done */
      iFlags |= A52_ADJUST_LEVEL;

      m_dll.a52_frame(m_pState, m_inputBuffer, &iFlags, &level, bias);

      // [a52_dynrng (state, ...); this is only optional]
      //int iNrOfBlocks = m_dll.a52_blocks_num(m_pState);
      int iNrOfBlocks = 6;
      for (int i = 0; i < iNrOfBlocks; i++)
      {
        if (m_dll.a52_block(m_pState) != 0)
        {
          CLog::Log(LOGDEBUG, "Not a valid AC3 frame");
          m_pInputBuffer    = m_inputBuffer;
          m_iFrameSize      = 0;
          m_DecoderError = true;
          return (pData - pOldDataPointer);
        }

        m_fSamples = m_dll.a52_samples(m_pState);
        m_decodedDataSize += 2*resample_int16(m_fSamples, (int16_t*)(m_decodedData + m_decodedDataSize), m_iOutputMapping);
      }
      m_pInputBuffer = m_inputBuffer;
      m_iFrameSize   = 0;
      return (pData - pOldDataPointer);
    }
  }
  return (pData - pOldDataPointer);
}

void AC3Codec::SetDefault()
{
  m_pInputBuffer      = m_inputBuffer;
  m_iFrameSize        = 0;
  m_iSourceChannels   = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate    = 0;
  m_decodedDataSize   = 0;
  m_readBufferPos     = 0;
  m_iDataStart        = -1;
}

#endif


