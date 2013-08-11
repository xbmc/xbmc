#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DynamicDll.h"

class DllStSoundInterface
{
public:
    virtual ~DllStSoundInterface() {}
    virtual void* LoadYM(const char* szFileName)=0;
    virtual void FreeYM(void* ym)=0;
    virtual int FillBuffer(void* ym, char* buffer, unsigned long length)=0;
    virtual unsigned long Seek(void* ym, unsigned long iTimePos)=0;
    virtual const char* GetTitle(void* ym)=0;
    virtual const char* GetArtist(void* ym)=0;
    virtual unsigned long GetLength(void* ym)=0;
};

class DllStSound : public DllDynamic, DllStSoundInterface
{
  DECLARE_DLL_WRAPPER(DllStSound, DLL_PATH_YM_CODEC)
  DEFINE_METHOD1(void*, LoadYM, (const char* p1))
  DEFINE_METHOD1(void, FreeYM, (void* p1))
  DEFINE_METHOD3(int, FillBuffer, (void* p1, char* p2, unsigned long p3))
  DEFINE_METHOD2(unsigned long, Seek, (void* p1, unsigned long p2))
  DEFINE_METHOD1(const char*, GetTitle, (void* p1))
  DEFINE_METHOD1(const char*, GetArtist, (void* p1))
  DEFINE_METHOD1(unsigned long, GetLength, (void* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadYM, LoadYM)
    RESOLVE_METHOD_RENAME(DLL_FreeYM, FreeYM)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};

