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

class DllWMAInterface
{
public:
    virtual ~DllWMAInterface() { }
    virtual int Init()=0;
    virtual void* LoadFile(const char* szFileName, long long* totalTime, int *sampleRate, int* bitsPerSample, int *channels)=0;
    virtual void UnloadFile(void* hnd)=0;
    virtual int FillBuffer(void* hnd, char* buffer, int length)=0;
    virtual unsigned long Seek(void* hnd, unsigned long iTimePos)=0;
};

class DllWMA : public DllDynamic, DllWMAInterface
{
  DECLARE_DLL_WRAPPER(DllWMA, DLL_PATH_WMA_CODEC)
  DEFINE_METHOD0(int, Init)
  DEFINE_METHOD5(void*, LoadFile, (const char* p1, long long* p2, int *p3, int* p4, int *p5))
  DEFINE_METHOD1(void, UnloadFile, (void* p1))
  DEFINE_METHOD3(int, FillBuffer, (void* p1, char* p2, int p3))
  DEFINE_METHOD2(unsigned long, Seek, (void* p1, unsigned long p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_Init, Init)
    RESOLVE_METHOD_RENAME(DLL_LoadFile, LoadFile)
    RESOLVE_METHOD_RENAME(DLL_UnloadFile, UnloadFile)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
  END_METHOD_RESOLVE()
};
