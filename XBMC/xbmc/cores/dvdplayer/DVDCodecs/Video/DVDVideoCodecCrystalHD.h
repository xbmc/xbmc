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

#ifndef HAVE_MPCLINK
#define HAVE_MPCLINK
#endif

#if defined(HAVE_MPCLINK)
#pragma once

#include "DVDVideoCodec.h"

namespace BCM
{
#if defined(WIN32)
  //#define _BC_DTS_TYPES_H_
  //typedef unsigned __int64  	U64;
  //typedef unsigned int        U32;
  //typedef int                 S32;
  //typedef unsigned short      U16;
  //typedef short               S16;
  //typedef unsigned char       U8;
  //typedef char                S8;
  typedef void		*HANDLE;
  #include "lib/crystalhd/include/windows/bc_drv_if.h"
  #include "lib/crystalhd/include/bc_dts_defs.h"
#else
  #ifndef __LINUX_USER__
  #define __LINUX_USER__
  #endif //__LINUX_USER__
  #if defined(__APPLE__)
    #include "bc_dts_types.h" 
    #include "bc_dts_defs.h" 
    #include "bc_ldil_if.h" 
  #else 
    #include "lib/crystalhd/include/linux/bc_ldil_if.h"
    #include "lib/crystalhd/include/linux/bc_dts_defs.h"
  #endif //defined(__APPLE__)
  #endif //defined(WIN32)
};

class CMPCDecodeBuffer
{
public:
  CMPCDecodeBuffer(size_t size);
  CMPCDecodeBuffer(unsigned char* pBuffer, size_t size);
  virtual ~CMPCDecodeBuffer();
  size_t GetSize();
  unsigned char* GetPtr();
  unsigned int GetId() {return m_Id;}
  void SetPts(double pts);
  double GetPts();
protected:
  size_t m_Size;
  unsigned char* m_pBuffer;
  unsigned int m_Id;
  static unsigned int m_NextId;
  double m_Pts;
};

#include <deque>
#include <vector>
#include "Thread.h"


class CMPCInputThread : public CThread
{
public:
  CMPCInputThread(BCM::HANDLE device);
  virtual ~CMPCInputThread();
  bool AddInput(unsigned char* pData, size_t size, double pts);
  unsigned int GetInputCount();
protected:
  CMPCDecodeBuffer* AllocBuffer(size_t size);
  void FreeBuffer(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer* GetNext();
  virtual void Process();

  CCriticalSection m_InputLock;
  std::deque<CMPCDecodeBuffer*> m_InputList;
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
protected:
  virtual void Process();
  CMPCDecodeBuffer* AllocBuffer();
  void AddFrame(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer* GetDecoderOutput();
  
  CCriticalSection m_FreeLock;
  CCriticalSection m_ReadyLock;
  
  std::deque<CMPCDecodeBuffer*> m_FreeList;
  std::deque<CMPCDecodeBuffer*> m_ReadyList;
  unsigned int m_BufferCount;
  
  unsigned int m_OutputHeight;
  unsigned int m_OutputWidth;
  BCM::HANDLE m_Device;
  unsigned int m_OutputTimeout;
};

class CDVDVideoCodecCrystalHD : public CDVDVideoCodec
{
public:
  CDVDVideoCodecCrystalHD();
  virtual ~CDVDVideoCodecCrystalHD();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double pts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return (const char*)m_pFormatName; }

protected:
  BCM::HANDLE m_Device;
  unsigned int m_Height;
  unsigned int m_Width;
  BCM::U32 m_YSize;
  BCM::U32 m_UVSize;
  BCM::U8* m_pBuffer;
  BCM::BC_DTS_PROC_OUT m_Output;
  bool m_DropPictures;
  unsigned int m_PicturesDecoded;
  unsigned int m_LastDecoded;
  char* m_pFormatName;

  BCM::BC_PIC_INFO_BLOCK m_CurrentFormat;
  unsigned int m_PacketsIn;
  unsigned int m_FramesOut;
  double m_LastPts;
  
  CMPCOutputThread* m_pOutputThread;
  CMPCInputThread* m_pInputThread;
  std::deque<CMPCDecodeBuffer*> m_BusyList;
};

#endif
