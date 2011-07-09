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

#if defined TSREADER && defined LIVE555

#include "os-dependent.h"
//#include <winsock2.h>
//#include <ws2tcpip.h>
#include "MemorySink.h"
#include "GroupsockHelper.hh"
#include "AutoLock.h"
#include "client.h"

#define SUBMIT_BUF_SIZE (1316*30)

CMemorySink::CMemorySink(UsageEnvironment& env,CMemoryBuffer& buffer, unsigned bufferSize) 
  : MediaSink(env),  
  fBufferSize(bufferSize),
  m_buffer(buffer)
{
  XBMC->Log(LOG_DEBUG, "CMemorySink::ctor");
  fBuffer = new unsigned char[bufferSize];
  m_pSubmitBuffer = new byte[SUBMIT_BUF_SIZE];
  m_iSubmitBufferPos=0;
  m_bReEntrant=false;
}

CMemorySink::~CMemorySink() 
{
  XBMC->Log(LOG_DEBUG, "CMemorySink::dtor");
  delete[] fBuffer;
  delete[] m_pSubmitBuffer;
}

CMemorySink* CMemorySink::createNew(UsageEnvironment& env, CMemoryBuffer& buffer,unsigned bufferSize) 
{
  return new CMemorySink(env, buffer,bufferSize);
}

Boolean CMemorySink::continuePlaying() 
{
  if (fSource == NULL) return False;

  fSource->getNextFrame(fBuffer, fBufferSize,afterGettingFrame, this,onSourceClosure, this);
  return True;
}

void CMemorySink::afterGettingFrame(void* clientData, unsigned frameSize,unsigned /*numTruncatedBytes*/,struct timeval presentationTime,unsigned /*durationInMicroseconds*/) 
{
  CMemorySink* sink = (CMemorySink*)clientData;
  sink->afterGettingFrame1(frameSize, presentationTime);
  sink->continuePlaying();
} 
static int testsize=0;
void CMemorySink::addData(unsigned char* data, unsigned dataSize,struct timeval presentationTime) 
{
  if (testsize ==0)
  {
    XBMC->Log(LOG_DEBUG, "CMemorySink:addData");
    testsize=1;
  }
  if (dataSize==0) return;
  if (data==NULL) return;
  if (m_bReEntrant)
  {
    XBMC->Log(LOG_DEBUG, "REENTRANT IN MEMORYSINK.CPP");
    return;
  }
  CAutoLock BufferLock(&m_BufferLock);
  m_bReEntrant=true;

  m_buffer.PutBuffer(data, dataSize);

  m_bReEntrant=false;
}


void CMemorySink::afterGettingFrame1(unsigned frameSize,struct timeval presentationTime) 
{
  addData(fBuffer, frameSize, presentationTime);
}
#endif //TSREADER
