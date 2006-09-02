
#include "../../../../stdafx.h"
#include "DVDAudioCodecLiba52.h"

// forward declarations
void fast_memcpy(void* d, const void* s, unsigned n);


typedef __int16 int16_t;
typedef __int32 int32_t;

static inline __int16 convert(int32_t i)
{
#ifdef LIBA52_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

static inline void convert2s16_2 (sample_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
    {
      s16[2*i] = convert (f[i]);
      s16[2*i+1] = convert (f[i+256]);
    }
}

static inline void convert2s16_4 (sample_t * _f, int16_t * s16)
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

static inline void convert2s16_5 (sample_t * _f, int16_t * s16)
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

static inline void convert2s16_multi (sample_t * _f, int16_t * s16, int flags)
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


CDVDAudioCodecLiba52::CDVDAudioCodecLiba52() : CDVDAudioCodec()
{
  m_pState = NULL;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
  m_pInputBuffer = NULL;
}

CDVDAudioCodecLiba52::~CDVDAudioCodecLiba52()
{
  Dispose();
}

bool CDVDAudioCodecLiba52::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize)
{
  if (!m_dll.Load()) return false;

  SetDefault();
  
  m_pState = m_dll.a52_init(0);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.a52_samples(m_pState);

  // set desired output
  m_iOutputChannels = iChannels;

  return true;
}

void CDVDAudioCodecLiba52::Dispose()
{
  if (m_pState) m_dll.a52_free(m_pState);
  m_pState = NULL;
}

#define HEADER_SIZE 7
int CDVDAudioCodecLiba52::Decode(BYTE* pData, int iSize)
{
  // channel lookup table
  /*
  static const int ac3Channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

    int i, len;

    BYTE* pBuf = pData;
    while (iSize > 0)
    {
        len = m_pInputBuffer - m_inputBuffer;
        if (m_iFrameSize == 0)
        {
            // no header seen : find one. We need at least 7 bytes to parse it
            len = HEADER_SIZE - len;
            if (len > iSize) len = iSize;
            memcpy(m_pInputBuffer, pBuf, len);
            pBuf += len;
            m_pInputBuffer += len;
            iSize -= len;
            if ((m_pInputBuffer - m_inputBuffer) == HEADER_SIZE)
            {
                len = m_dll.a52_syncinfo(m_inputBuffer, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate);
                if (len == 0)
                {
                   // no sync found : move by one byte (inefficient, but simple!)
                    memcpy(m_inputBuffer, m_inputBuffer + 1, HEADER_SIZE - 1);
                    m_pInputBuffer--;
                }
                else
                {
                  m_iFrameSize = len;
                    // update codec info
                    m_iSourceChannels = ac3Channels[m_iFlags & 7];
                    if (m_iFlags & A52_LFE) m_iSourceChannels++;
                  if (m_iChannels == 0)
                  {
                   // No specific number of channel requested
                   m_iChannels = m_iSourceChannels;
                 }
                }
            }
        }
        else if (len < m_iFrameSize)
        {
            len = m_iFrameSize - len;
            if (len > iSize) len = iSize;

            memcpy(m_pInputBuffer, pBuf, len);
            pBuf += len;
            m_pInputBuffer += len;
            iSize -= len;
        }
        else // we have at least one frame to decode
        {
            int iFlags = m_iFlags;
            if (m_iChannels == 1)
                iFlags = A52_MONO;
            else if (m_iChannels == 2)
                iFlags = A52_STEREO;
            else
                iFlags |= A52_ADJUST_LEVEL;

            float fLevel = 1.0f;
            if (m_dll.a52_frame(m_pState, m_inputBuffer, &iFlags, &fLevel, 384))
            {
            fail:
                m_pInputBuffer = m_inputBuffer;
                m_iFrameSize = 0;
                continue;
            }
            for (i = 0; i < 6; i++)
            {
                if (m_dll.a52_block(m_pState) != 0) goto fail;
                float_to_int(m_fSamples, (short*)m_decodedData + i * 256 * m_iChannels, m_iChannels);
            }
            m_pInputBuffer = m_inputBuffer;
            m_iFrameSize = 0;
            m_decodedDataSize = 6 * m_iChannels * 256 * sizeof(short);
            break;
        }
    }
    return pBuf - pData;
  */
  static const int ac3Channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

  int iLen = 0;
  m_decodedDataSize = 0;
  BYTE* pOldDataPointer = pData;
  while (iSize > 0)
  {
    if (m_iFrameSize == 0)
    {
      // no header seen : find one. We need at least 7 bytes to parse it
      int i = 0;
      while (i <= (iSize - 7))
      {
        // it's possible that m_inputBuffer already contains 6 bits from our previous run
        // so use m_pInputBuffer to copy the rest of the data. We must rest it after a52_syncinfo though!!
        for (int u = 0; u < 7; u++) m_pInputBuffer[u] = pData[u];

        iLen = m_dll.a52_syncinfo(m_inputBuffer, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate);
        if (iLen > 0)
        {
          if (m_iSourceChannels == 0)
          {
            m_iSourceChannels = ac3Channels[m_iFlags & 7];
            if (m_iFlags & A52_LFE) m_iSourceChannels++;
          }
          m_iFrameSize = iLen;
          pData += 7;
          m_pInputBuffer += 7;
          iSize -= 7;
          break;
        }

        // reset the buffer pointer if needed
        if ((m_pInputBuffer - m_inputBuffer) > 0) m_pInputBuffer = m_inputBuffer;

        // no sync found, shift one byte
        i++;
        pData++;
        //m_pInputBuffer++;
        iSize--;
      }
      if (m_iFrameSize == 0 && iSize < 7)
      {
        // we are at the end of our stream and don't have enough data for our header anymore.
        // copy it to our buffer for later use;
        for (int i = 0; i < iSize; i++) m_pInputBuffer[i] = pData[i];
        m_pInputBuffer += iSize;
        pData += iSize;
        iSize = 0;
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
      pData += iLen;
      iSize -= iLen;
    }
    else
    {
      // we have a frame to decode
      float fLevel = 1.0f;
      int iFlags = m_iFlags;

      if (m_iOutputChannels == 1)
        iFlags = A52_MONO;
      else if (m_iOutputChannels == 2)
        iFlags = A52_STEREO; /* this could be set to A52_DOLBY, to keep the suround effects */
      else
      {
        if(m_iOutputChannels > 0 && m_iOutputChannels != m_iSourceChannels)
          CLog::Log(LOGINFO, __FUNCTION__" - Requested output channels (%d), differs from source (%d). Using source's channels", m_iOutputChannels, m_iSourceChannels);

        m_iOutputChannels = m_iSourceChannels;
      };

      /* adjust level should always be set, to keep samples in proper range */
      /* after any downmixing has been done */
      iFlags |= A52_ADJUST_LEVEL;

      m_dll.a52_frame(m_pState, m_inputBuffer, &iFlags, &fLevel, 384);

      // [a52_dynrng (state, ...); this is only optional]
      for (int i = 0; i < 6; i++)
      {
        if (m_dll.a52_block(m_pState) != 0)
        {
          CLog::Log(LOGERROR, "CDVDAudioCodecLiba52::Decode - a52_block failed");
          m_pInputBuffer = m_inputBuffer;
          m_iFrameSize = 0;
          m_decodedDataSize = 0;
          return -1;
        }

        convert2s16_multi(m_fSamples, (short*)(m_decodedData + m_decodedDataSize), iFlags & (A52_CHANNEL_MASK | A52_LFE));
        m_decodedDataSize += 256 * sizeof(short) * m_iOutputChannels;
      }

      m_pInputBuffer = m_inputBuffer;
      m_iFrameSize = 0;      
      return (pData - pOldDataPointer);
    }
  }
  return (pData - pOldDataPointer);
}


int CDVDAudioCodecLiba52::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_decodedDataSize;
}

void CDVDAudioCodecLiba52::SetDefault()
{
  m_pInputBuffer = m_inputBuffer;
  m_iFrameSize = 0;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
}

void CDVDAudioCodecLiba52::Reset()
{
  if (m_pState) m_dll.a52_free(m_pState);

  SetDefault();

  m_pState = m_dll.a52_init(0);
  m_fSamples = m_dll.a52_samples(m_pState);
}

int CDVDAudioCodecLiba52::GetChannels()
{
  return m_iOutputChannels;
}

int CDVDAudioCodecLiba52::GetSampleRate()
{
  return m_iSourceSampleRate;
}

int CDVDAudioCodecLiba52::GetBitsPerSample()
{
  return 16;
}
