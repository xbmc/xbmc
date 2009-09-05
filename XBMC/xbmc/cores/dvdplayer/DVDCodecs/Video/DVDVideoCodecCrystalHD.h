/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#pragma once
#if defined(HAVE_LIBCRYSTALHD)

#include "DVDVideoCodec.h"

namespace BCM
{
#if defined(WIN32)
  typedef void		*HANDLE;
  #include "lib/crystalhd/include/bc_dts_defs.h"
  #include "lib/crystalhd/include/windows/bc_drv_if.h"
#else
  #ifndef __LINUX_USER__
  #define __LINUX_USER__
  #endif

  #include "crystalhd/bc_dts_types.h"
  #include "crystalhd/bc_dts_defs.h"
  #include "crystalhd/bc_ldil_if.h"
#endif //defined(WIN32)
};

extern const char* g_DtsStatusText[];
void PrintFormat(BCM::BC_PIC_INFO_BLOCK& pib);

class CMPCDecodeBuffer
{
public:
  CMPCDecodeBuffer(size_t size);
  CMPCDecodeBuffer(unsigned char* pBuffer, size_t size);
  virtual ~CMPCDecodeBuffer();
  size_t GetSize();
  unsigned char* GetPtr();
  void SetPts(BCM::U64 pts);
  BCM::U64 GetPts();
protected:
  size_t m_Size;
  unsigned char* m_pBuffer;
  unsigned int m_Id;
  BCM::U64 m_Pts;
};

#include <deque>
#include <vector>
#include <utils/Thread.h>

template <class T>
class CSyncPtrQueue
{
public:
  CSyncPtrQueue()
  {
    InitializeCriticalSection(&m_Lock);
  }
  virtual ~CSyncPtrQueue()
  {
    DeleteCriticalSection(&m_Lock);
  }
  void Push(T* p)
  {
    EnterCriticalSection(&m_Lock);
    m_Queue.push_back(p);
    LeaveCriticalSection(&m_Lock);
  }
  T* Pop()
  {
    T* p = NULL;
    EnterCriticalSection(&m_Lock);
    if (m_Queue.size())
    {
      p = m_Queue.front();
      m_Queue.pop_front();
    }
    LeaveCriticalSection(&m_Lock);
    return p;
  }
  unsigned int Count(){return m_Queue.size();}
protected:
  std::deque<T*> m_Queue;
  CRITICAL_SECTION m_Lock;
};

class CMPCInputThread : public CThread
{
public:
  CMPCInputThread(BCM::HANDLE device);
  virtual ~CMPCInputThread();
  bool AddInput(unsigned char* pData, size_t size, BCM::U64 pts);
  unsigned int GetQueueLen();
protected:
  CMPCDecodeBuffer* AllocBuffer(size_t size);
  void FreeBuffer(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer* GetNext();
  virtual void Process();

  CSyncPtrQueue<CMPCDecodeBuffer> m_InputList;
  BCM::HANDLE m_Device;
};

class CMPCOutputThread : public CThread
{
public:
  CMPCOutputThread(BCM::HANDLE device);
  virtual ~CMPCOutputThread();
  unsigned int GetReadyCount();
  CMPCDecodeBuffer* GetNext();
  void FreeBuffer(CMPCDecodeBuffer* pBuffer);
  bool WaitForPicture(unsigned int timeout);
protected:
  virtual void Process();
  CMPCDecodeBuffer* AllocBuffer();
  void AddFrame(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer* GetDecoderOutput();
  
  CSyncPtrQueue<CMPCDecodeBuffer> m_FreeList;
  CSyncPtrQueue<CMPCDecodeBuffer> m_ReadyList;

  BCM::HANDLE m_Device;
  unsigned int m_OutputHeight;
  unsigned int m_OutputWidth;
  unsigned int m_OutputTimeout;
  unsigned int m_BufferCount;
  unsigned int m_PictureCount;
};

class CDVDVideoCodecCrystalHD : public CDVDVideoCodec
{
public:
  CDVDVideoCodecCrystalHD();
  virtual ~CDVDVideoCodecCrystalHD();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int  Decode(BYTE* pData, int iSize, double pts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual bool ReleasePicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return (const char*)m_pFormatName; }

protected:
  BCM::HANDLE m_Device;
  unsigned int m_Height;
  unsigned int m_Width;
  BCM::U32 m_YSize;
  BCM::U32 m_UVSize;
  BCM::U8* m_pBuffer;
  bool m_DropPictures;
  unsigned int m_PicturesDecoded;
  const char* m_pFormatName;

  unsigned int m_PacketsIn;
  unsigned int m_FramesOut;
  
  CMPCOutputThread* m_pOutputThread;
  CMPCInputThread* m_pInputThread;
  CSyncPtrQueue<CMPCDecodeBuffer> m_BusyList;
};

#endif
