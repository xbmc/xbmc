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

class DllGensApuInterface
{
public:
    virtual ~DllGensApuInterface() {}
    virtual int Init()=0;
    virtual int LoadGYM(const char* szFileName)=0;
    virtual void FreeGYM(int gym)=0;
    virtual int FillBuffer(int gym, void* buffer)=0;
    virtual void Seek(int gym, unsigned int iPos)=0;
    virtual int GetTitle(int spc)=0;
    virtual int GetArtist(int spc)=0;
    virtual __int64 GetLength(int gym)=0;
};

class DllGensApu : public DllDynamic, DllGensApuInterface
{
  DECLARE_DLL_WRAPPER(DllGensApu, DLL_PATH_GYM_CODEC)
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadGYM, (const char* p1))
  DEFINE_METHOD1(void, FreeGYM, (int p1))
  DEFINE_METHOD2(int, FillBuffer, (int p1, void* p2))
  DEFINE_METHOD2(void, Seek, (int p1, unsigned int p2))
  DEFINE_METHOD1(int, GetTitle, (int p1))
  DEFINE_METHOD1(int, GetArtist, (int p1))
  DEFINE_METHOD1(__int64, GetLength, (int p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadGYM, LoadGYM)
    RESOLVE_METHOD_RENAME(DLL_FreeGYM, FreeGYM)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};
