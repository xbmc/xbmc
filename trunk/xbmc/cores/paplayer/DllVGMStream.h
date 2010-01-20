#pragma once

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

#include "DynamicDll.h"

class DllVGMInterface
{
public:
    virtual ~DllVGMInterface() {}
    virtual long Init()=0;
    virtual long LoadVGM(const char* szFileName, int* sampleRate, int* samleSize, int* channels)=0;
    virtual void FreeVGM(long vgm)=0;
    virtual int FillBuffer(long vgm, char* buffer, int length)=0;
    virtual unsigned long Seek(long vgm, unsigned long iTimePos)=0;
    virtual unsigned long GetLength(long vgm)=0;
};

class DllVGM : public DllDynamic, DllVGMInterface
{
  DECLARE_DLL_WRAPPER(DllVGM, DLL_PATH_VGM_CODEC)
  DEFINE_METHOD0(long, Init)
  DEFINE_METHOD4(long, LoadVGM, (const char* p1, int* p2, int* p3, int* p4 ))
  DEFINE_METHOD1(void, FreeVGM, (long p1))
  DEFINE_METHOD3(int, FillBuffer, (long p1, char* p2, int p3))
  DEFINE_METHOD2(unsigned long, Seek, (long p1, unsigned long p2))
  DEFINE_METHOD1(unsigned long, GetLength, (long p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadVGM, LoadVGM)
    RESOLVE_METHOD_RENAME(DLL_FreeVGM, FreeVGM)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};

