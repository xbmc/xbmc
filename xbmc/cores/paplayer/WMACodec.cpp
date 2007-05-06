#include "stdafx.h"
#ifdef HAS_WMA_CODEC
#include "WMACodec.h"
#include "../../Util.h"
#include "../../utils/Win32Exception.h"

DWORD CALLBACK WMAStreamCallback( VOID* pContext, ULONG offset, ULONG num_bytes,
                                  VOID** ppData )
{
    WMAInfo *pThis = (WMAInfo*)pContext;

    if (offset != pThis->fileReader.GetPosition())
      pThis->fileReader.Seek(offset,SEEK_SET);
    
    pThis->fileReader.Read(pThis->buffer,num_bytes);
    *ppData = pThis->buffer;
    return num_bytes;
}


WMACodec::WMACodec()
{
  m_pWMA = NULL;
  m_CodecName = "WMA";
  m_iDataPos = -1; 
}

WMACodec::~WMACodec()
{
  DeInit();
}

bool WMACodec::Init(const CStdString &strFile, unsigned int filecache)
{
  WAVEFORMATEX wfxSourceFormat;

  m_info.fileReader.Initialize(filecache);
  if (!m_info.fileReader.Open(strFile))
    return false;
  
  m_info.iStartOfBuffer = -1;

  try
  {
    HRESULT hr = WmaCreateInMemoryDecoder( WMAStreamCallback, &m_info, 0,
                                    &wfxSourceFormat, (LPXMEDIAOBJECT*)&m_pWMA);
  
    if (FAILED(hr))
      return false;
  
    WMAXMOFileHeader info;
    m_pWMA->GetFileHeader(&info);
    m_Channels = info.dwNumChannels;
    m_SampleRate = info.dwSampleRate;
    m_BitsPerSample = 16;
    m_TotalTime = info.dwDuration; // fixme?
    m_iDataPos = 0;
    m_iDataInBuffer = 0;    
  }
  catch(win32_exception e)
  {
    e.writelog(__FUNCTION__);
    return false;
  }

  return true;
}

void WMACodec::DeInit()
{
  if (m_pWMA)
    m_pWMA->Release();
}

__int64 WMACodec::Seek(__int64 iSeekTime)
{
  DWORD dwResult;
  m_pWMA->SeekToTime((DWORD)iSeekTime,&dwResult);
  return dwResult;
}

int WMACodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  unsigned int iSizeToGo = size;
  DWORD iPacketSize;
  XMEDIAPACKET xmp;
  ZeroMemory( &xmp, sizeof(xmp) );
  xmp.pvBuffer = m_buffer;
  xmp.dwMaxSize = 2048*2*m_Channels;
  xmp.pdwCompletedSize = &iPacketSize;
  BYTE* pStartOfBuffer = pBuffer;
  while (iSizeToGo > 0)
  {        
    if (m_iDataInBuffer == 0)
    {
      HRESULT hr = m_pWMA->Process(NULL,&xmp);
      if (FAILED(hr) || iPacketSize == 0)
      {
        *actualsize = size-iSizeToGo;
        return READ_EOF;
      }
      m_iDataInBuffer = iPacketSize;
      m_startOfBuffer = m_buffer;
    }
    int iCopy=m_iDataInBuffer>iSizeToGo?iSizeToGo:m_iDataInBuffer;
    memcpy(pStartOfBuffer,m_startOfBuffer,iCopy);
    iSizeToGo -= iCopy;
    pStartOfBuffer += iCopy;
    m_iDataInBuffer -= iCopy;
    m_startOfBuffer += iCopy;
  }
  
  *actualsize = size-iSizeToGo;

  return READ_SUCCESS;
}

bool WMACodec::CanInit()
{
  return true;
}

#endif

