#include "../../stdafx.h"
#include "AC3Codec.h"


static inline int16_t convert(int32_t i)
{
#ifdef LIBA52_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void AC3Codec::convert2s16_2 (sample_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
    {
      s16[2*i] = convert (f[i]);
      s16[2*i+1] = convert (f[i+256]);
    }
}

void AC3Codec::convert2s16_4 (sample_t * _f, int16_t * s16)
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

void AC3Codec::convert2s16_5 (sample_t * _f, int16_t * s16)
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

void AC3Codec::convert2s16_multi (sample_t * _f, int16_t * s16, int flags)
{
  int i;
  int32_t * f = (int32_t *) _f;

  switch (flags)
  {
    case A52_MONO:
      for (i = 0; i < 256; i++)
      {
        s16[5*i] = s16[5*i+1] = s16[5*i+2] = s16[5*i+3] = 0;
        s16[5*i+4] = convert (f[i]);
      }
      break;
    case A52_CHANNEL:
    case A52_STEREO:
    case A52_DOLBY:
      convert2s16_2 (_f, s16);
      break;
    case A52_3F:
      for (i = 0; i < 256; i++)
      {
        s16[5*i] = convert (f[i]);
        s16[5*i+1] = convert (f[i+512]);
        s16[5*i+2] = s16[5*i+3] = 0;
        s16[5*i+4] = convert (f[i+256]);
      }
      break;
    case A52_2F2R:
      convert2s16_4 (_f, s16);
      break;
    case A52_3F2R:
      convert2s16_5 (_f, s16);
      break;
    case A52_MONO | A52_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i+256]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case A52_CHANNEL | A52_LFE:
    case A52_STEREO | A52_LFE:
    case A52_DOLBY | A52_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case A52_3F | A52_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+768]);
        s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i+512]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case A52_2F2R | A52_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = convert (f[i+768]);
        s16[6*i+3] = convert (f[i+1024]);
        s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case A52_3F2R | A52_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+768]);
        s16[6*i+2] = convert (f[i+1024]);
        s16[6*i+3] = convert (f[i+1280]);
        s16[6*i+4] = convert (f[i+512]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
  }
}

AC3Codec::AC3Codec()
{
  m_CodecName         = L"AC3";
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
  m_file.Initialize(filecache);
  //  Open the file to play
  return m_file.Open(strFile, false);
}

bool AC3Codec::CalculateTotalTime()
{
  //m_TotalTime = (((m_file.GetLength() - m_iDataStart) * 8) / m_Bitrate) * 1000;
  m_TotalTime = (m_file.GetLength() - m_iDataStart) / (m_SampleRate * 2 * (m_BitsPerSample / 8)) * 1000;
  m_Bitrate = (int)(((m_file.GetLength() - m_iDataStart) * 8) / float(m_TotalTime / 1000));
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

  Decode(m_readBuffer, m_readBufferPos);
  if (m_decodedDataSize == 0)
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

  m_file.StartBuffering();
  m_IsInitialized = true;
  return true;
}

void AC3Codec::CloseFile()
{
  m_file.Close();
}

int AC3Codec::GetNrOfChannels(int flags)
{
  static const int ac3Channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
  int nr = ac3Channels[m_iFlags & 7];
  if (m_iFlags & A52_LFE) nr++;
  return nr;
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
  level_t level = 1.0f;
  sample_t bias = 384;
  
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

        iLen = m_dll.a52_syncinfo(m_pState, m_inputBuffer, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate);
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
      fast_memcpy(m_pInputBuffer, pData, iLen);
      
      m_pInputBuffer += iLen;
      pData          += iLen;
      iSize          -= iLen;
    }
    else
    {
      // we have a frame to decode
      int iFlags = m_iFlags;
      if (m_iOutputChannels == 1)      iFlags = A52_MONO;
      else if (m_iOutputChannels == 2) iFlags = A52_STEREO;
      else
      {
        m_iOutputChannels = m_iSourceChannels;
        iFlags |= A52_ADJUST_LEVEL;
      }

      m_dll.a52_frame(m_pState, m_inputBuffer, &iFlags, &level, bias);

      // [a52_dynrng (state, ...); this is only optional]
      //int iNrOfBlocks = m_dll.a52_blocks_num(m_pState);
      int iNrOfBlocks = 6;
      for (int i = 0; i < iNrOfBlocks; i++)
      {
        if (m_dll.a52_block(m_pState) != 0)
        {
          OutputDebugString("Not a valid AC3 frame\n");
          m_pInputBuffer    = m_inputBuffer;
          m_iFrameSize      = 0;
          m_decodedDataSize = 0;
          return -1;
        }
        
        m_fSamples = m_dll.a52_samples(m_pState);
        convert2s16_multi(m_fSamples, (short*)(m_decodedData + m_decodedDataSize), iFlags & (A52_CHANNEL_MASK | A52_LFE));
        m_decodedDataSize += 256 * sizeof(short) * m_iOutputChannels;
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