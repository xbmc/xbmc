/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef TSREADER

#include "os-dependent.h"
#include "MemoryBuffer.h"
#include "AutoLock.h"
#include "client.h"

#define MAX_MEMORY_BUFFER_SIZE (1024L*1024L*12L)

CMemoryBuffer::CMemoryBuffer(void) :m_event(NULL,FALSE,FALSE,NULL)
{
  //XBMC->Log(LOG_DEBUG, "CMemoryBuffer::ctor");
  m_bRunning=true;
  m_BytesInBuffer=0;
  m_pcallback=NULL;
}

CMemoryBuffer::~CMemoryBuffer()
{
  //XBMC->Log(LOG_DEBUG, "CMemoryBuffer::dtor");
  Clear();
}

bool CMemoryBuffer::IsRunning()
{
  return m_bRunning;
}

void CMemoryBuffer::Clear()
{
  //XBMC->Log(LOG_DEBUG, "memorybuffer: Clear() %d",m_Array.size());
  CAutoLock BufferLock(&m_BufferLock);
  std::vector<BUFFERITEM *>::iterator it = m_Array.begin();
  for ( ; it != m_Array.end() ; it++ )
  {
    BUFFERITEM *item = *it;
    delete[] item->data;
    delete item;
  }
  m_Array.clear();
  m_BytesInBuffer=0;
  //XBMC->Log(LOG_DEBUG, "memorybuffer: Clear() done");
}

unsigned long CMemoryBuffer::Size()
{
  return m_BytesInBuffer;
}

void CMemoryBuffer::Run(bool onOff)
{
  //XBMC->Log(LOG_DEBUG, "memorybuffer: run:%d %d", onOff, m_bRunning);
  if (m_bRunning!=onOff)
  {
    m_bRunning=onOff;
    if (m_bRunning==false) 
    {
      Clear();
    }
  }
  //XBMC->Log(LOG_DEBUG, "memorybuffer: running:%d", onOff);
}

unsigned long CMemoryBuffer::ReadFromBuffer(unsigned char *pbData, long lDataLength)
{  
  if (pbData==NULL) return 0;
  if (lDataLength<=0) return 0;
  if (!m_bRunning) return 0;
  while (m_BytesInBuffer < (unsigned long) lDataLength)
  {
    if (!m_bRunning) return 0;
    m_event.ResetEvent();
    m_event.Wait();
    if (!m_bRunning) return 0;
  }

  //Log("get..%d/%d",lDataLength,m_BytesInBuffer);
  long bytesWritten = 0;
  CAutoLock BufferLock(&m_BufferLock);
  while (bytesWritten < lDataLength)
  {
    if(!m_Array.size() || m_Array.size() <= 0)
    {
      XBMC->Log(LOG_DEBUG, "memorybuffer: read:empty buffer\n");
      return 0;
    }
    BUFFERITEM *item = m_Array.at(0);
    
    long copyLength = min(item->nDataLength - item->nOffset, lDataLength-bytesWritten);
    memcpy(&pbData[bytesWritten], &item->data[item->nOffset], copyLength);

    bytesWritten += copyLength;
    item->nOffset += copyLength;
    m_BytesInBuffer-=copyLength;

    if (item->nOffset >= item->nDataLength)
    {
      m_Array.erase(m_Array.begin());
      delete[] item->data;
      delete item;
    }
  }
  return bytesWritten;
}

long CMemoryBuffer::PutBuffer(unsigned char *pbData, long lDataLength)
{
  if (lDataLength<=0) return E_FAIL;
  if (pbData==NULL) return E_FAIL;

  BUFFERITEM* item = new BUFFERITEM();
  item->nOffset=0;
  item->nDataLength=lDataLength;
  item->data = new byte[lDataLength];
  memcpy(item->data, pbData, lDataLength);
  bool sleep=false;
  {
    CAutoLock BufferLock(&m_BufferLock);
    m_Array.push_back(item);
    m_BytesInBuffer+=lDataLength;

    //Log("add..%d/%d",lDataLength,m_BytesInBuffer);
    while (m_BytesInBuffer > MAX_MEMORY_BUFFER_SIZE)
    {
      sleep=true;
      XBMC->Log(LOG_DEBUG, "memorybuffer:put full buffer (%d)",m_BytesInBuffer);
      BUFFERITEM *item = m_Array.at(0);
      int copyLength=item->nDataLength - item->nOffset;

      m_BytesInBuffer-=copyLength;
      m_Array.erase(m_Array.begin());
      delete[] item->data;
      delete item;
    }
    if (m_BytesInBuffer>0)
    {
      m_event.SetEvent();
    }
  }
  if (m_pcallback)
  {
    m_pcallback->OnRawDataReceived(pbData,lDataLength);
  }
  if (sleep)
  {
    Sleep(10);
  }
  return S_OK;
}


void CMemoryBuffer::SetCallback(IMemoryCallback* callback)
{
  m_pcallback=callback;
}

#endif //TSREADER