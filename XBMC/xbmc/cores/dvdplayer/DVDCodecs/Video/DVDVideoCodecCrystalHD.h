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

#define _BC_DTS_TYPES_H_
#ifdef WIN32
    typedef unsigned __int64  	U64;
#else
    typedef unsigned long long  U64;
#endif
typedef unsigned int		U32;
typedef int					S32;
typedef unsigned short  	U16;
typedef short				S16;
typedef unsigned char		U8;
typedef char				S8;


#if defined(WIN32)
#include "lib/crystalhd/include/windows/bc_drv_if.h"
#else
#ifndef __LINUX_USER__
#define __LINUX_USER__
#endif //__LINUX_USER__
#if defined(__APPLE__)
#include "bc_dts_defs.h" 
#include "bc_ldil_if.h" 
#else 
#include "lib/crystalhd/include/linux/bc_ldil_if.h"
#include "lib/crystalhd/include/linux/bc_dts_defs.h"
#endif //defined(__APPLE__)
#endif //defined(WIN32)

#include "LockFree.h"

class CMPCDecodeBuffer
{
public:
  CMPCDecodeBuffer(size_t size);
  CMPCDecodeBuffer(unsigned char* pBuffer, size_t size);
  virtual ~CMPCDecodeBuffer();
  size_t GetSize();
  unsigned char* GetPtr();
  unsigned int GetId() {return m_Id;}
protected:
  size_t m_Size;
  unsigned char* m_pBuffer;
  unsigned int m_Id;
  static unsigned int m_NextId;
};

#include <deque>
#include <vector>
#include "Thread.h"

class CMPCDecodeThread : public CThread
{
public:
#ifdef __APPLE__
  CMPCDecodeThread(void* device);
#else
  CMPCDecodeThread(HANDLE device);
#endif
  virtual ~CMPCDecodeThread();
  unsigned int GetReadyCount();
  CMPCDecodeBuffer* GetNext();
  void FreeBuffer(CMPCDecodeBuffer* pBuffer);
  bool AddInput(CMPCDecodeBuffer* pBuffer);
  unsigned int GetInputCount();
protected:
  virtual void Process();
  CMPCDecodeBuffer* AllocBuffer();
  void AddFrame(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer* GetDecoderOutput();
  

  CCriticalSection m_InputLock;
  CCriticalSection m_FreeLock;
  CCriticalSection m_ReadyLock;
  
  std::deque<CMPCDecodeBuffer*> m_InputList;
  std::deque<CMPCDecodeBuffer*> m_FreeList;
  std::deque<CMPCDecodeBuffer*> m_ReadyList;
  unsigned int m_BufferCount;
  
  unsigned int m_OutputHeight;
  unsigned int m_OutputWidth;
#ifdef __APPLE__
  void* m_Device;
#else
  HANDLE m_Device;
#endif
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
#ifdef __APPLE__
  void* m_Device;
#else
  HANDLE m_Device;
#endif
  unsigned int m_Height;
  unsigned int m_Width;
  U32 m_YSize;
  U32 m_UVSize;
  U8* m_pBuffer;
  BC_DTS_PROC_OUT m_Output;
  bool m_DropPictures;
  unsigned int m_PicturesDecoded;
  unsigned int m_LastDecoded;
  char* m_pFormatName;

  BC_PIC_INFO_BLOCK m_CurrentFormat;
  unsigned int m_PacketsIn;
  unsigned int m_FramesOut;
  unsigned int m_OutputTimeout;
  double m_LastPts;
  
  CMPCDecodeThread* m_pDecodeThread;
  std::deque<CMPCDecodeBuffer*> m_BusyList;
};

#endif
