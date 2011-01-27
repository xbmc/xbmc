#ifndef SPC_CODEC_H_
#define SPC_CODEC_H_

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

#include "ICodec.h"
#include "snesapu/Types.h"
#include "../DllLoader/LibraryLoader.h"

class SPCCodec : public ICodec
{
public:
  SPCCodec();
  virtual ~SPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
#ifdef _LINUX
  typedef void  (__cdecl *LoadMethod) ( const void* p1);
  typedef void* (__cdecl *EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__cdecl *SeekMethod) ( u32 p1, b8 p2 );
  typedef u32 (__cdecl *InitMethod)(void);
  typedef void (__cdecl *DeInitMethod)(void);
#else
  typedef void  (__stdcall* LoadMethod) ( const void* p1);
  typedef void* (__stdcall * EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__stdcall * SeekMethod) ( u32 p1, b8 p2 );
#endif
  struct
  {
    LoadMethod LoadSPCFile;
    EmuMethod EmuAPU;
    SeekMethod SeekAPU;
#ifdef _LINUX
    InitMethod InitAPU;
    DeInitMethod ResetAPU;
#endif
  } m_dll;

  LibraryLoader* m_loader;
  CStdString m_loader_name;

  char* m_szBuffer;
  u8* m_pApuRAM;
  __int64 m_iDataPos;
};

#endif
