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

#include "../DVDVideoCodec.h"

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
  CPictureBuffer(DVDVideoPicture::EFormat format, int width, int height);
  virtual ~CPictureBuffer();

  unsigned int  m_width;
  unsigned int  m_height;
  unsigned int  m_field;
  bool          m_interlace;
  double        m_framerate;
  uint64_t      m_timestamp;
  unsigned int  m_color_range;
  unsigned int  m_color_matrix;
  uint64_t      m_PictureNumber;
  DVDVideoPicture::EFormat m_format;
  unsigned char *m_y_buffer_ptr;
  unsigned char *m_u_buffer_ptr;
  unsigned char *m_v_buffer_ptr;
  unsigned char *m_uv_buffer_ptr;
  int           m_y_buffer_size;
  int           m_u_buffer_size;
  int           m_v_buffer_size;
  int           m_uv_buffer_size;
};


////////////////////////////////////////////////////////////////////////////////////////////
enum _CRYSTALHD_CODEC_TYPES
{
  CRYSTALHD_CODEC_ID_MPEG2 = 0,
  CRYSTALHD_CODEC_ID_H264  = 1,
  CRYSTALHD_CODEC_ID_VC1   = 2,
  CRYSTALHD_CODEC_ID_WMV3  = 3,
};

typedef uint32_t CRYSTALHD_CODEC_TYPE;
////////////////////////////////////////////////////////////////////////////////////////////

#define CRYSTALHD_FIELD_FULL        0x00
#define CRYSTALHD_FIELD_EVEN        0x01
#define CRYSTALHD_FIELD_ODD         0x02

typedef struct CHD_TIMESTAMP
{
  double dts;
  double pts;
} CHD_TIMESTAMP;

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

  bool OpenDecoder(CRYSTALHD_CODEC_TYPE codec_type, int extradata_size, void *extradata);
  void CloseDecoder(void);
  void Reset(void);

  bool WaitInput(unsigned int msec);
  bool AddInput(unsigned char *pData, size_t size, double dts, double pts);

  int  GetInputCount(void);
  int  GetReadyCount(void);
  void ReadyListPop(void);
  void BusyListFlush(void);

  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);

protected:
  void CheckCrystalHDLibraryPath(void);

  DllLibCrystalHD *m_dll;
  void          *m_Device;

  bool          m_IsConfigured;
  bool          m_drop_state;
  unsigned int  m_OutputTimeout;
  unsigned int  m_field;
  unsigned int  m_width;
  unsigned int  m_height;

  std::deque<CHD_TIMESTAMP> m_timestamps;
  CMPCInputThread *m_pInputThread;
  CMPCOutputThread *m_pOutputThread;
  CSyncPtrQueue<CPictureBuffer> m_BusyList;

private:
  CCrystalHD();
  static CCrystalHD *m_pInstance;
};

#endif
