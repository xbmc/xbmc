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
#include "utils/log.h"
#ifdef USE_LIBDTS_DECODER
#include "DTSCodec.h"

static inline int16_t convert(int32_t i)
{
#ifdef LIBDTS_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void DTSCodec::convert2s16_2 (sample_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
    {
      s16[2*i] = convert (f[i]);
      s16[2*i+1] = convert (f[i+256]);
    }
}

void DTSCodec::convert2s16_4 (sample_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
    {
      s16[4*i] = convert (f[i]);
      s16[4*i+1] = convert (f[i+256]);
      s16[4*i+2] = convert (f[i+512]);
      s16[4*i+3] = convert (f[i+768]);
    }
}

void DTSCodec::convert2s16_5 (sample_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
    {
      s16[5*i] = convert (f[i+256]);
      s16[5*i+1] = convert (f[i+512]);
      s16[5*i+2] = convert (f[i+768]);
      s16[5*i+3] = convert (f[i+1024]);
      s16[5*i+4] = convert (f[i]);
    }
}

void DTSCodec::convert2s16_multi (sample_t * _f, int16_t * s16, int flags)
{
  int i;
  int32_t * f = (int32_t *) _f;

  switch (flags)
    {
    case DTS_MONO:   //MON 2
      for (i = 0; i < 256; i++)
        {
          s16[2*i] = s16[2*i+1] = convert (f[i]);
        }
      break;
    case DTS_CHANNEL:   //L R 2
    case DTS_STEREO:
    case DTS_STEREO_TOTAL:
      convert2s16_2 (_f, s16);
      break;
    case DTS_STEREO_SUMDIFF:  //L+R L-R 2
      for (i = 0; i < 256; i++)
        {
          s16[2*i] = convert (f[i])/2 + convert (f[i+256])/2;
          s16[2*i+1] = convert (f[i])/2 - convert (f[i+256])/2;
        }
      break;
    case DTS_3F:   //C L R 5
      for (i = 0; i < 256; i++)
        {
          s16[5*i] = convert (f[i+256]);
          s16[5*i+1] = convert (f[i+512]);
          s16[5*i+2] = s16[5*i+3] = 0;
          s16[5*i+4] = convert (f[i]);
        }
      break;
    case DTS_2F1R:   //L R S 4
      for (i = 0; i < 256; i++)
        {
          s16[4*i] = convert (f[i]);
          s16[4*i+1] = convert (f[i+256]);
          s16[4*i+2] = s16[4*i+3] = convert (f[i+512]);
        }
      break;
    case DTS_3F1R:   //C L R S 5
      for (i = 0; i < 256; i++)
        {
          s16[5*i] = convert (f[i]+256);
          s16[5*i+1] = convert (f[i+512]);
          s16[5*i+2] = s16[5*i+3] = convert (f[i+768]);
          s16[5*i+4] = convert (f[i]);
        }
      break;
    case DTS_2F2R:   //L R SL SR 4
      convert2s16_4 (_f, s16);
      break;
    case DTS_3F2R:   //C L R SL SR 5
      convert2s16_5 (_f, s16);
      break;
    case DTS_MONO | DTS_LFE:  //MON LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
          s16[6*i+4] = convert (f[i]);
          s16[6*i+5] = convert (f[i+256]);
        }
      break;
    case DTS_CHANNEL | DTS_LFE:
    case DTS_STEREO | DTS_LFE:
    case DTS_STEREO_TOTAL | DTS_LFE: //L R LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i]);
          s16[6*i+1] = convert (f[i+256]);
          s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
          s16[6*i+5] = convert (f[i+512]);
        }
      break;
    case DTS_STEREO_SUMDIFF | DTS_LFE: //L+R L-R LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i])/2 + convert (f[i+256])/2;
          s16[6*i+1] = convert (f[i])/2 - convert (f[i+256])/2;
          s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
          s16[6*i+5] = convert (f[i+512]);
        }
      break;
    case DTS_3F | DTS_LFE: //C L R LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i+256]);
          s16[6*i+1] = convert (f[i+512]);
          s16[6*i+2] = s16[6*i+3] = 0;
          s16[6*i+4] = convert (f[i]);
          s16[6*i+5] = convert (f[i+768]);
        }
      break;
    case DTS_2F1R | DTS_LFE: //L R S LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i]);
          s16[6*i+1] = convert (f[i+256]);
          s16[6*i+2] = s16[6*i+3] = convert (f[i+512]);
          s16[6*i+4] = 0;
          s16[6*i+5] = convert (f[i+768]);
        }
      break;
    case DTS_3F1R | DTS_LFE: //C L R S LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i+256]);
          s16[6*i+1] = convert (f[i+512]);
          s16[6*i+2] = s16[6*i+3] = convert (f[i+768]);
          s16[6*i+4] = convert (f[i]);
          s16[6*i+5] = convert (f[i+1024]);
        }
      break;
    case DTS_2F2R | DTS_LFE: //L R SL SR LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i]);
          s16[6*i+1] = convert (f[i+256]);
          s16[6*i+2] = convert (f[i+512]);
          s16[6*i+3] = convert (f[i+768]);
          s16[6*i+4] = 0;
          s16[6*i+5] = convert (f[i+1024]);
        }
      break;
    case DTS_3F2R | DTS_LFE: //C L R SL SR LFE 6
      for (i = 0; i < 256; i++)
        {
          s16[6*i] = convert (f[i+256]);
          s16[6*i+1] = convert (f[i+512]);
          s16[6*i+2] = convert (f[i+768]);
          s16[6*i+3] = convert (f[i+1024]);
          s16[6*i+4] = convert (f[i]);
          s16[6*i+5] = convert (f[i+1280]);

        }
      break;
    }
}

DTSCodec::DTSCodec()
{
  m_CodecName         = "DTS";
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
}

DTSCodec::~DTSCodec()
{
  DeInit();
}

void DTSCodec::DeInit()
{
  m_IsInitialized = false;
  if (m_pState) m_dll.dts_free(m_pState);
  m_pState = NULL;

  CloseFile();
  m_readBufferPos = 0;
  if (m_readBuffer)  SAFE_DELETE_ARRAY(m_readBuffer);
  if (m_decodedData) SAFE_DELETE_ARRAY(m_decodedData);
}

__int64 DTSCodec::Seek(__int64 iSeekTime)
{
  //  Calculate size of a single sample of the file
  int iSampleSize = m_SampleRate * 2 * (m_BitsPerSample / 8);
  //  Seek to the position in the file
  __int64 newPos = m_file.Seek(m_iDataStart + ((iSeekTime / 1000) * iSampleSize));
  m_readBufferPos = 0;
  m_iFrameSize    = 0;
  if (newPos == -1)
    m_eof = true;

  return iSeekTime;
}

bool DTSCodec::CanInit()
{
  return m_dll.CanLoad();
}


void DTSCodec::PrepairBuffers()
{
  m_decodingBufferSize = 131072;
  m_readingBufferSize  = 98304;
  m_decodedData        = new BYTE[m_decodingBufferSize];
  m_readBuffer         = new BYTE[m_readingBufferSize];
  m_readBufferPos      = m_file.Read(m_readBuffer, m_readingBufferSize); // big read, much crap sometimes before data starts....
}

bool DTSCodec::InitFile(const CStdString &strFile, unsigned int filecache)
{
  //  Open the file to play
  return m_file.Open(strFile, READ_CACHED);
}

bool DTSCodec::CalculateTotalTime()
{
  //libdts gives half the actual bitrate sometime causing the total time to double
  //don't know why this is, vlc has the same problem
  m_TotalTime = (((m_file.GetLength() - m_iDataStart) * 8) / m_Bitrate) * 1000;
  return m_TotalTime > 0;
}

bool DTSCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (m_IsInitialized) return true;
  if (!InitFile(strFile, filecache))
  {
    CLog::Log(LOGERROR, "DTSCodec: Can't open %s", strFile.c_str());
    return false;
  }
  if (!m_dll.Load())
    return false;

  SetDefault();

  m_pState = m_dll.dts_init(0);
  if (!m_pState)
    return false;

  m_eof = false;

  PrepairBuffers();

  int istart = 0;
  do
  {
    m_iDataStart = -1;
    Decode(m_readBuffer + istart, m_readBufferPos - istart);
    //decoding error, could still be a valid dts-stream, try again at next byte
    istart += std::max(m_iDataStart + 1, 1);
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

void DTSCodec::CloseFile()
{
  m_file.Close();
}

int DTSCodec::GetNrOfChannels(int iFlags)
{
  int iChannels = 0;
  
  switch (iFlags & DTS_CHANNEL_MASK)
  {
    case DTS_MONO:           iChannels =  1; break;
    case DTS_CHANNEL:        iChannels =  1; break; // ???
    case DTS_STEREO:         iChannels =  2; break;
    case DTS_STEREO_SUMDIFF: iChannels =  2; break;
    case DTS_STEREO_TOTAL:   iChannels =  2; break;
    case DTS_3F:             iChannels =  3; break;
    case DTS_2F1R:           iChannels =  3; break;
    case DTS_3F1R:           iChannels =  4; break;
    case DTS_2F2R:           iChannels =  4; break;
    case DTS_3F2R:           iChannels =  5; break;
    case DTS_4F2R:           iChannels =  6; break;
    default: iChannels = 0; break;
  }
  if (iFlags & DTS_LFE) iChannels++;
  
  return iChannels;
}

int DTSCodec::ReadInput()
{
  int inputBufferToRead = m_readingBufferSize - m_readBufferPos;
  if ( inputBufferToRead && !m_eof ) 
  {
    int fileLeft=(int)(m_file.GetLength() - m_file.GetPosition());
    if (inputBufferToRead >  fileLeft ) inputBufferToRead = fileLeft;

    DWORD dwBytesRead = m_file.Read(m_readBuffer + m_readBufferPos, inputBufferToRead);
    if (!dwBytesRead)
    {
      CLog::Log(LOGERROR, "DTSCodec: Error reading file");
      return -1;
    }
    m_readBufferPos += dwBytesRead;
    if ( m_file.GetLength() == m_file.GetPosition() )
      m_eof = true;
    return dwBytesRead;
  }
  return 0;
}

int DTSCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
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
    if ((m_readBufferPos > 0) && (m_decodedDataSize < m_decodingBufferSize - 16384))
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

#define HEADER_SIZE 14
int DTSCodec::Decode(BYTE* pData, int iSize)
{
  level_t level = 1.0f;
  sample_t bias = 384;
  m_DecoderError = false;

  int iLen = 0;
  BYTE* pOldDataPointer = pData;
  while (iSize > 0)
  {
    if (m_iFrameSize == 0)
    {
      // no header seen : find one. We need at least 7 bytes to parse it
      int i = 0;
      while (i <= (iSize - HEADER_SIZE))
      {
        // it's possible that m_inputBuffer already contains 6 bits from our previous run
        // so use m_pInputBuffer to copy the rest of the data. We must rest it after dts_syncinfo though!!
        for (int u = 0; u < HEADER_SIZE; u++) m_pInputBuffer[u] = pData[u];

        iLen = m_dll.dts_syncinfo(m_pState, m_inputBuffer, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iFrameSize);
        if (iLen > 0)
        {
          if (m_iSourceChannels == 0)
          {
            m_iSourceChannels = GetNrOfChannels(m_iFlags);
          }
          m_iFrameSize = iLen;
          //start of data in file
          if (m_iDataStart == -1)
            m_iDataStart = (pData - pOldDataPointer);
          
          pData          += HEADER_SIZE;
          m_pInputBuffer += HEADER_SIZE;
          iSize          -= HEADER_SIZE;
          break;
        }

        // reset the buffer pointer if needed
        if ((m_pInputBuffer - m_inputBuffer) > 0) m_pInputBuffer = m_inputBuffer;

        // no sync found, shift one byte
        i++;
        pData++;
        iSize--;
      }
      if (m_iFrameSize == 0 && iSize < HEADER_SIZE)
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
      //m_iOutputChannels = 2;
      int iFlags = m_iFlags;
      if (m_iOutputChannels == 1)      iFlags = DTS_MONO;
      else if (m_iOutputChannels == 2) iFlags = DTS_STEREO;
      else
      {
        m_iOutputChannels = m_iSourceChannels;
        iFlags |= DTS_ADJUST_LEVEL;
      }

      m_dll.dts_frame(m_pState, m_inputBuffer, &iFlags, &level, bias);

      // [dts_dynrng (state, ...); this is only optional]
      int iNrOfBlocks = m_dll.dts_blocks_num(m_pState);
      for (int i = 0; i < iNrOfBlocks; i++)
      {
        if (m_dll.dts_block(m_pState) != 0)
        {
          CLog::Log(LOGDEBUG, "Not a valid DTS frame");
          m_pInputBuffer = m_inputBuffer;
          m_iFrameSize   = 0;
          m_DecoderError = true;
          return (pData - pOldDataPointer);
        }
        m_fSamples = m_dll.dts_samples(m_pState);
        convert2s16_multi(m_fSamples, (short*)(m_decodedData + m_decodedDataSize), iFlags & (DTS_CHANNEL_MASK | DTS_LFE));
        m_decodedDataSize += 256 * sizeof(short) * m_iOutputChannels;
      }
      m_pInputBuffer = m_inputBuffer;
      m_iFrameSize   = 0;
      return (pData - pOldDataPointer);
    }
  }
  return (pData - pOldDataPointer);
}

void DTSCodec::SetDefault()
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


