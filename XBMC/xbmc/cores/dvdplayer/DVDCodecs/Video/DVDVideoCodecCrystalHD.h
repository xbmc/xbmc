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

#pragma once

#if defined(WIN32)
// The Broadcom headers collide with existing defines. Explicitly define those that are needed.
#define _BC_DTS_TYPES_H_
typedef unsigned __int64  U64;
typedef unsigned int   U32;
typedef int            S32;
typedef unsigned short U16;
typedef short          S16;
typedef unsigned char  U8;
typedef char           S8;
#include "lib/crystalhd/include/windows/bc_drv_if.h"
#else
#ifndef __LINUX_USER__
#define __LINUX_USER__
#endif
#include "lib/crystalhd/include/linux/bc_dts_defs.h"
#include "lib/crystalhd/include/linux/bc_ldil_if.h"
#endif

#include "DVDVideoCodec.h"
#include <fstream>

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
  bool IsPictureReady();
  HANDLE m_Device;
  BC_DTS_PROC_OUT m_Output;
  bool m_DropPictures;
  unsigned int m_PicturesDecoded;
  unsigned int m_LastDecoded;
  char* m_pFormatName;

  BC_PIC_INFO_BLOCK m_CurrentFormat;

  unsigned int m_FramesOut;
  unsigned int m_OutputTimeout;
  double m_LastPts;

  std::fstream m_DumpFile;
};