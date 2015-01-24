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

class DllNosefartInterface
{
public:
    virtual ~DllNosefartInterface() {}
    virtual void* LoadNSF(const char* szFileName)=0;
    virtual void FreeNSF(void*)=0;
    virtual int StartPlayback(void* nsf, int track)=0;
    virtual long FillBuffer(void* nsf, char* buffer, int size)=0;
    virtual void FrameAdvance(void* nsf)=0;
    virtual int GetPlaybackRate(void* nsf)=0;
    virtual int GetNumberOfSongs(void* nsf)=0;
    virtual char* GetTitle(void* nsf)=0;
    virtual char* GetArtist(void* nsf)=0;
};

class DllNosefart : public DllDynamic, DllNosefartInterface
{
  DECLARE_DLL_WRAPPER(DllNosefart, DLL_PATH_NSF_CODEC)
  DEFINE_METHOD1(void*, LoadNSF, (const char* p1))
  DEFINE_METHOD1(void, FreeNSF, (void* p1))
  DEFINE_METHOD2(int, StartPlayback, (void* p1, int p2))
  DEFINE_METHOD3(long, FillBuffer, (void* p1, char* p2, int p3))
  DEFINE_METHOD1(void, FrameAdvance, (void* p1))
  DEFINE_METHOD1(int, GetPlaybackRate, (void* p1))
  DEFINE_METHOD1(int, GetNumberOfSongs, (void* p1))
  DEFINE_METHOD1(char*, GetTitle, (void* p1))
  DEFINE_METHOD1(char*, GetArtist, (void* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadNSF, LoadNSF)
    RESOLVE_METHOD_RENAME(DLL_FreeNSF, FreeNSF)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback, StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_FrameAdvance, FrameAdvance)
    RESOLVE_METHOD_RENAME(DLL_GetPlaybackRate, GetPlaybackRate)
    RESOLVE_METHOD_RENAME(DLL_GetNumberOfSongs, GetNumberOfSongs)
    RESOLVE_METHOD_RENAME(DLL_GetTitle, GetTitle)
    RESOLVE_METHOD_RENAME(DLL_GetArtist, GetArtist)
  END_METHOD_RESOLVE()
};
