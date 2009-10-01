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

class DllSidplay2Interface
{
public:
    virtual ~DllSidplay2Interface() {}
    virtual int Init()=0;
    virtual int LoadSID(const char* szFileName)=0;
    virtual void FreeSID(int sid)=0;
    virtual void StartPlayback(int sid, int track)=0;
    virtual int FillBuffer(int sid, void* buffer, int length)=0;
    virtual int GetNumberOfSongs(const char* szFileName)=0;
    virtual void SetSpeed(int sid, int speed)=0;
};

class DllSidplay2 : public DllDynamic, DllSidplay2Interface
{
  DECLARE_DLL_WRAPPER(DllSidplay2, DLL_PATH_SID_CODEC)
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD1(int, LoadSID, (const char* p1))
  DEFINE_METHOD1(void, FreeSID, (int p1))
  DEFINE_METHOD2(void, StartPlayback,(int p1, int p2))
  DEFINE_METHOD3(int, FillBuffer, (int p1, void* p2, int p3))
  DEFINE_METHOD1(int, GetNumberOfSongs, (const char* p1))
  DEFINE_METHOD2(void, SetSpeed, (int p1, int p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadSID, LoadSID)
    RESOLVE_METHOD_RENAME(DLL_FreeSID, FreeSID)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback,StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_GetNumberOfSongs, GetNumberOfSongs)
    RESOLVE_METHOD_RENAME(DLL_SetSpeed, SetSpeed)
  END_METHOD_RESOLVE()
};

