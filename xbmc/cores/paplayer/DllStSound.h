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

class DllStSoundInterface
{
public:
    virtual ~DllStSoundInterface() {}
    virtual int LoadYM(const char* szFileName)=0;
    virtual void FreeYM(int ym)=0;
    virtual int FillBuffer(int ym, char* buffer, unsigned long length)=0;
    virtual unsigned long Seek(int ym, unsigned long iTimePos)=0;
    virtual const char* GetTitle(int ym)=0;
    virtual const char* GetArtist(int ym)=0;
    virtual unsigned long GetLength(int ym)=0;
};

class DllStSound : public DllDynamic, DllStSoundInterface
{
  DECLARE_DLL_WRAPPER(DllStSound, DLL_PATH_YM_CODEC)
  DEFINE_METHOD1(int, LoadYM, (const char* p1))
  DEFINE_METHOD1(void, FreeYM, (int p1))
  DEFINE_METHOD3(int, FillBuffer, (int p1, char* p2, unsigned long p3))
  DEFINE_METHOD2(unsigned long, Seek, (int p1, unsigned long p2))
  DEFINE_METHOD1(const char*, GetTitle, (int p1))
  DEFINE_METHOD1(const char*, GetArtist, (int p1))
  DEFINE_METHOD1(unsigned long, GetLength, (int p1))
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

