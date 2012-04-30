#pragma once
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

#if defined LIVE555

#ifndef _MEDIA_SINK_HH
  #include "MediaSink.hh" //Live555 header
#endif

#include "MemoryBuffer.h"
#include "platform/threads/mutex.h"

class CMemorySink: public MediaSink
{
  public:
    static CMemorySink* createNew(UsageEnvironment& env, CMemoryBuffer& buffer, unsigned bufferSize = 20000);
    void addData(unsigned char* data, unsigned dataSize, struct timeval presentationTime);

  protected:
    CMemorySink(UsageEnvironment& env, CMemoryBuffer& buffer, unsigned bufferSize = 20000);
    virtual ~CMemorySink(void);

    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
    virtual void afterGettingFrame1(unsigned frameSize,struct timeval presentationTime);

    unsigned char* fBuffer;
    unsigned fBufferSize;
    CMemoryBuffer& m_buffer;

  private: // redefined virtual functions:
    virtual Boolean continuePlaying();

    PLATFORM::CMutex m_BufferLock;
    unsigned char* m_pSubmitBuffer;
    int   m_iSubmitBufferPos;
    bool  m_bReEntrant;
};
#endif //LIVE555
