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

#include "stdafx.h"
#ifdef HAS_WMA_CODEC
#include "WMACodec.h"
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

  if (!m_info.fileReader.Open(strFile, READ_CACHED)) 
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
    return false;   // We always ask ffmpeg to return s16le
  }   m_BitsPerSample = 16;

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

