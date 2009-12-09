#pragma once
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

#if defined(HAVE_LIBCRYSTALHD)

#include <deque>
#include <vector>

#include "DVDVideoCodec.h"
#include "utils/CriticalSection.h"
namespace Surface { class CSurface; }

////////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////////
class CMPCDecodeBuffer
{
public:
  CMPCDecodeBuffer(size_t size);
  CMPCDecodeBuffer(unsigned char* pBuffer, size_t size);
  virtual ~CMPCDecodeBuffer();
  size_t GetSize();
  unsigned char* GetPtr();
  void SetPts(uint64_t pts);
  uint64_t GetPts();
protected:
  size_t m_Size;
  unsigned char* m_pBuffer;
  unsigned int m_Id;
  uint64_t m_Pts;
};

/* We really don't want to include ffmpeg headers, so define these */
enum _BCM_CODEC_TYPES
{
  BC_CODEC_ID_MPEG2 = 2,
  BC_CODEC_ID_H264  = 28,
  BC_CODEC_ID_VC1   = 73,
};
enum _BCM_STREAM_TYPE
{
	BC_STREAM_TYPE_ES         = 0,
	BC_STREAM_TYPE_PES        = 1,
	BC_STREAM_TYPE_TS         = 2,
	BC_STREAM_TYPE_ES_TSTAMP	= 6,
};

typedef uint32_t BCM_CODEC_TYPE;
typedef uint32_t BCM_STREAM_TYPE;
////////////////////////////////////////////////////////////////////////////////////////////
class CMPCInputThread;
class CMPCOutputThread;
typedef struct YV12Image YV12Image;

class CCrystalHD
{
public:
  CCrystalHD();
  virtual ~CCrystalHD();

  bool Open(BCM_STREAM_TYPE stream_type, BCM_CODEC_TYPE codec_type);
  void Close(void);
  void Flush(void);
  unsigned int GetInputCount();
  bool AddInput(unsigned char *pData, size_t size, double pts);
  unsigned int GetReadyCount();
  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  bool FreePicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);

protected:
  void*     m_dl_handle;
  void*     m_Device;
  
  bool      InitHardware(void);

  //Display*  m_Display;
  bool      m_IsConfigured;
  bool      m_Inited;
  bool      m_drop_state;
  const char* m_pFormatName;
  bool      m_interlace;
  double    m_framerate;

  CMPCInputThread* m_pInputThread;
  CMPCOutputThread* m_pOutputThread;
  CSyncPtrQueue<CMPCDecodeBuffer> m_BusyList;
};

extern CCrystalHD*          g_CrystalHD;

#endif
