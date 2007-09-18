
#include "stdafx.h"
#include "DVDAudioCodecLiba52.h"

#ifndef _LINUX
typedef __int16 int16_t;
typedef __int32 int32_t;
#endif

static inline __int16 convert(int32_t i)
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
    for (i = 0; i != 256; i++) {
	unsigned long map = channel_map;
	do {
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


CDVDAudioCodecLiba52::CDVDAudioCodecLiba52() : CDVDAudioCodec()
{
  m_pState = NULL;
  m_iSourceFlags = 0;
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
  
  m_pState = m_dll.a52_init(MM_ACCEL_X86_MMX |MM_ACCEL_X86_MMXEXT);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.a52_samples(m_pState);

  // output will be decided on when
  // we have decoded something
  m_iOutputChannels = iChannels;

  return true;
}

void CDVDAudioCodecLiba52::Dispose()
{
  if (m_pState) m_dll.a52_free(m_pState);
  m_pState = NULL;
}

void CDVDAudioCodecLiba52::SetupChannels()
{
  // setup channel map for how to translate to linear format
  // standard windows format
  if(m_iSourceFlags & A52_LFE)
  {
    switch (m_iSourceFlags&~A52_LFE) {
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
    switch (m_iSourceFlags) {
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

  if(m_iOutputChannels > 0 && m_iOutputChannels != channels)
    CLog::Log(LOGINFO, "%s - Number of channels changed in stream from %d to %d, data might be truncated", __FUNCTION__, m_iOutputChannels, channels);

  m_iOutputChannels = channels;

  // make sure map contains enough channels
  for(int i=0;i<m_iOutputChannels;i++)
  {
    if((m_iOutputMapping & (0xf<<(i*4))) == 0)
      m_iOutputMapping |= 0xf<<(i*4);
  }
  // and nothing more
  m_iOutputMapping &= ~(0xffffffff<<(m_iOutputChannels*4));
}

int CDVDAudioCodecLiba52::Decode(BYTE* pData, int iSize)
{
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

        int flags;
        iLen = m_dll.a52_syncinfo(m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
        if (iLen > 0)
        {
          if(flags != m_iSourceFlags)
          {
            m_iSourceFlags = flags;
            SetupChannels();
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
      memcpy(m_pInputBuffer, pData, iLen);
      m_pInputBuffer += iLen;
      pData += iLen;
      iSize -= iLen;
    }
    else
    {
      // we have a frame to decode

      
      float fLevel = 1.0f;      
      int iFlags = m_iSourceFlags;

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
 
        m_decodedDataSize += 2*resample_int16(m_fSamples, (int16_t*)(m_decodedData + m_decodedDataSize/2), m_iOutputMapping);
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
  *dst = (BYTE*)m_decodedData;
  return m_decodedDataSize;
}

void CDVDAudioCodecLiba52::SetDefault()
{
  m_pInputBuffer = m_inputBuffer;
  m_iFrameSize = 0;
  m_iSourceFlags = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
}

void CDVDAudioCodecLiba52::Reset()
{
  if (m_pState) m_dll.a52_free(m_pState);

  SetDefault();

  m_pState = m_dll.a52_init(MM_ACCEL_X86_MMX |MM_ACCEL_X86_MMXEXT); // MMX accel is not really important since liba52 doesnt yet implement this... but whatever...
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
