
#include "stdafx.h"
#include "DVDAudioCodecLibFaad.h"
#include "../../DVDStreamInfo.h"

CDVDAudioCodecLibFaad::CDVDAudioCodecLibFaad() : CDVDAudioCodec()
{
  m_bInitializedDecoder = false;
  
  m_pHandle = NULL;
}

CDVDAudioCodecLibFaad::~CDVDAudioCodecLibFaad()
{
  Dispose();
}

bool CDVDAudioCodecLibFaad::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // for safety
  if (m_pHandle) Dispose();
  
  if (!m_dll.Load())
    return false;
  
  memset(&m_frameInfo, 0, sizeof(m_frameInfo));
  
  if (!OpenDecoder() ) 
    return false;

  m_bRawAACStream = true;;

  if( hints.extrasize )
  {
    m_bRawAACStream = false;

    unsigned long samplerate;
    unsigned char channels;

    int res = m_dll.faacDecInit2(m_pHandle, (unsigned char*)hints.extradata, hints.extrasize, &samplerate, &channels);
    if (res < 0)
      return false;

    m_iSourceSampleRate = samplerate;
    m_iSourceChannels = channels;
    m_iSourceBitrate = 0;
  
    m_bInitializedDecoder = true;
  }
  return true;
}

void CDVDAudioCodecLibFaad::Dispose()
{
  CloseDecoder();
}

bool CDVDAudioCodecLibFaad::SyncStream()
{
  BYTE* p = m_InputBuffer;
  
  while (m_InputBufferSize > 4)
  {
    // Check if an ADIF or ADTS header is present
    if (((p[0] == 'A') && (p[1] == 'D') && (p[2] == 'I') && (p[3] == 'F')) ||
        ((p[1] | p[0] << 8) & 0xfff0) == 0xfff0)
    {
      // sync found, update our buffer if needed
      if (p != m_InputBuffer)
      {
        CLog::Log(LOGINFO, "CDVDAudioCodecLibFaad::SyncStream(), stream synced at offset %d", p - m_InputBuffer);
        memmove(m_InputBuffer, p, m_InputBufferSize);
      }
      return true;
    }
    p++;
    m_InputBufferSize--;
  }
  
  // no sync found
  CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad::SyncStream(), no sync found (ADIF or ADTS header) in stream");
  return false;
}

int CDVDAudioCodecLibFaad::Decode(BYTE* pData, int iSize)
{
  m_DecodedDataSize = 0;
  
  if (!m_pHandle)
    return -1;

  int iBytesToCopy = min(iSize, LIBFAAD_INPUT_SIZE - m_InputBufferSize);
  memcpy(m_InputBuffer + m_InputBufferSize, pData, iBytesToCopy);
  m_InputBufferSize += iBytesToCopy;
  
  // if the caller does not supply enough data, return
  if (m_InputBufferSize < FAAD_MIN_STREAMSIZE)
    return iBytesToCopy;
    
  if(m_bRawAACStream)
  {
    // attempt to sync stream
    if (!SyncStream())
      return iBytesToCopy;

    // initialize decoder if needed
    if (!m_bInitializedDecoder)
    {
      unsigned long samplerate;
      unsigned char channels;
      
      int res = m_dll.faacDecInit(m_pHandle, m_InputBuffer, m_InputBufferSize, &samplerate, &channels);
      if (res >= 0)
      {
        m_iSourceSampleRate = samplerate;
        m_iSourceChannels = channels;
        m_iSourceBitrate = 0;

        m_bInitializedDecoder = true;
      }
    }
  }

  // if we haven't succeded in initing now, keep going
  if (!m_bInitializedDecoder)
    return iBytesToCopy;

  m_DecodedData = (short*)m_dll.faacDecDecode(m_pHandle, &m_frameInfo, m_InputBuffer, m_InputBufferSize);
  m_DecodedDataSize = m_frameInfo.samples * sizeof(short);

  if (m_frameInfo.error)
  {    
    char* strError = m_dll.faacDecGetErrorMessage(m_frameInfo.error);
    m_dll.faacDecPostSeekReset(m_pHandle, 0);
    CLog::Log(LOGERROR, "CDVDAudioCodecLibFaad() : %s", strError);
    m_InputBufferSize = 0;
    return iBytesToCopy;
  }

  // we set this info again, it could be this info changed 
  m_iSourceSampleRate = m_frameInfo.samplerate;
  m_iSourceChannels = m_frameInfo.channels;
  m_iSourceBitrate = 0;
            
  // move remaining data along
  m_InputBufferSize -= m_frameInfo.bytesconsumed;
  memcpy(m_InputBuffer, m_InputBuffer+m_frameInfo.bytesconsumed, m_InputBufferSize);
  
  return iBytesToCopy;
}

int CDVDAudioCodecLibFaad::GetData(BYTE** dst)
{
  *dst = (BYTE*)m_DecodedData;
  return m_DecodedDataSize;
}

void CDVDAudioCodecLibFaad::Reset()
{
  if (m_pHandle)
    m_dll.faacDecPostSeekReset(m_pHandle, 0);
}

void CDVDAudioCodecLibFaad::CloseDecoder()
{
  if (m_pHandle)
  {
    m_dll.faacDecClose(m_pHandle);
    m_pHandle = NULL;
  }
}

bool CDVDAudioCodecLibFaad::OpenDecoder()
{
  if (m_pHandle)
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad : Decoder already opened");
    return false;
  }
  
  m_bInitializedDecoder = false;
  
  m_InputBufferSize = 0;
  m_DecodedDataSize = 0;
  
  m_iSourceSampleRate = 0;
  m_iSourceChannels = 0;
  m_iSourceBitrate = 0;
  
  m_pHandle = m_dll.faacDecOpen();
  
  if (m_pHandle)
  {
    faacDecConfigurationPtr pConfiguration;
    pConfiguration = m_dll.faacDecGetCurrentConfiguration(m_pHandle);

    // modify some stuff here
    pConfiguration->outputFormat = FAAD_FMT_16BIT; // already default

    m_dll.faacDecSetConfiguration(m_pHandle, pConfiguration);

    return true;
  }
  
  return false;
}
