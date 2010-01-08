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
class CPictureBuffer
{
public:
  CPictureBuffer(int ybuffsize, int uvbuffsize);
  virtual ~CPictureBuffer();

  unsigned int  m_width;
  unsigned int  m_height;
  unsigned int  m_field;
  bool          m_interlace;
  double        m_framerate;
  uint64_t      m_timestamp;
  unsigned int  m_PictureNumber;
  unsigned char *m_y_buffer_ptr;
  unsigned char *m_uv_buffer_ptr;
  int           m_y_buffer_size;
  int           m_uv_buffer_size;
};


////////////////////////////////////////////////////////////////////////////////////////////
/* We really don't want to include ffmpeg headers, so define these */
enum _CRYSTALHD_CODEC_TYPES
{
  CRYSTALHD_CODEC_ID_MPEG2 = 2,
  CRYSTALHD_CODEC_ID_H264  = 28,
  CRYSTALHD_CODEC_ID_VC1   = 73,
};
enum _CRYSTALHD_STREAM_TYPE
{
  CRYSTALHD_STREAM_TYPE_ES         = 0,
  CRYSTALHD_STREAM_TYPE_PES        = 1,
  CRYSTALHD_STREAM_TYPE_TS         = 2,
  CRYSTALHD_STREAM_TYPE_ES_TSTAMP  = 6,
};

typedef uint32_t CRYSTALHD_CODEC_TYPE;
typedef uint32_t CRYSTALHD_STREAM_TYPE;
////////////////////////////////////////////////////////////////////////////////////////////

#define CRYSTALHD_FIELD_FULL        0x00
#define CRYSTALHD_FIELD_EVEN        0x01
#define CRYSTALHD_FIELD_ODD         0x02

class DllLibCrystalHD;
class CMPCInputThread;
class CMPCOutputThread;

class CCrystalHD
{
public:
  virtual ~CCrystalHD();

  static void RemoveInstance(void);
  static CCrystalHD* GetInstance(void);

  bool DevicePresent(void);

  bool OpenDecoder(CRYSTALHD_CODEC_TYPE stream_type, CRYSTALHD_STREAM_TYPE codec_type);
  void CloseDecoder(void);
  bool IsOpenforDecode(void);
  void Flush(void);
  unsigned int GetInputCount(void);
  bool AddInput(unsigned char *pData, size_t size, double pts);

  int  GetReadyCount(void);
  void BusyListPop(void);

  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);

protected:
  void CheckCrystalHDLibraryPath(void);
  void SetFrameRate(uint32_t resolution);
  void SetAspectRatio(uint32_t aspect_ratio, uint32_t custom_aspect_ratio_width_height);

  DllLibCrystalHD *m_dll;
  void          *m_Device;

  bool          m_IsConfigured;
  bool          m_drop_state;
  int           m_ignore_drop_count;
  unsigned int  m_OutputTimeout;
  unsigned int  m_field;
  unsigned int  m_width;
  unsigned int  m_height;

  CMPCInputThread *m_pInputThread;
  CMPCOutputThread *m_pOutputThread;
  CSyncPtrQueue<CPictureBuffer> m_BusyList;

private:
  CCrystalHD();
  static CCrystalHD *m_pInstance;
};

#endif
