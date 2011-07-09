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

#if defined TSREADER

#include "WaitEvent.h"
#include "CritSec.h"
#include <vector>

using namespace std;

class IMemoryCallback
{
  public:
    virtual void OnRawDataReceived(unsigned char *pbData, long lDataLength) = 0;
};

class CMemoryBuffer
{
  public:
    CMemoryBuffer(void);
    virtual ~CMemoryBuffer(void);

    void SetCallback(IMemoryCallback* callback);
    unsigned long ReadFromBuffer(unsigned char *pbData, long lDataLength);
    long PutBuffer(unsigned char *pbData, long lDataLength);
    void Clear();
    unsigned long Size();
    void Run(bool onOff);
    bool IsRunning();

    struct BufferItem
    {
      unsigned char* data;
      int   nDataLength;
      int   nOffset;
    };
    typedef struct BufferItem BUFFERITEM;

  protected:
    vector<BUFFERITEM *> m_Array;
    CCritSec m_BufferLock;
    unsigned long    m_BytesInBuffer;
    CWaitEvent m_event;
    IMemoryCallback* m_pcallback;
    bool m_bRunning;
};

#endif //TSREADER