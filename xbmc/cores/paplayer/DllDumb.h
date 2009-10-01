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

#ifndef _XBOX
#ifdef FreeModule
#undef FreeModule
#endif
#endif

class DllDumbInterface
{
public:
    struct DUH * LoadModule(const char *szFileName);
    void FreeModule(struct DUH *duh);
    long GetModuleLength(struct DUH *duh);
    long GetModulePosition(struct DUH_SIGRENDERER *sig);
    struct DUH_SIGRENDERER * StartPlayback(struct DUH *duh, long pos);
    void StopPlayback(struct DUH_SIGRENDERER *sig);
    long FillBuffer(struct DUH_SIGRENDERER *sig, char *buffer, int size, float volume);
};

class DllDumb : public DllDynamic, DllDumbInterface
{
  DECLARE_DLL_WRAPPER(DllDumb, DLL_PATH_MODULE_CODEC)
  DEFINE_METHOD1(struct DUH *, LoadModule, (const char *p1))
  DEFINE_METHOD1(void, FreeModule, (struct DUH *p1))
  DEFINE_METHOD1(long, GetModuleLength, (struct DUH *p1))
  DEFINE_METHOD1(long, GetModulePosition, (struct DUH_SIGRENDERER *p1))
  DEFINE_METHOD2(struct DUH_SIGRENDERER *, StartPlayback, (struct DUH *p1, long p2))
  DEFINE_METHOD1(void, StopPlayback, (struct DUH_SIGRENDERER *p1))
  DEFINE_METHOD4(long, FillBuffer, (struct DUH_SIGRENDERER *p1, char* p2, int p3, float p4))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadModule, LoadModule)
    RESOLVE_METHOD_RENAME(DLL_FreeModule, FreeModule)
    RESOLVE_METHOD_RENAME(DLL_GetModuleLength, GetModuleLength)
    RESOLVE_METHOD_RENAME(DLL_GetModulePosition, GetModulePosition)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback, StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_StopPlayback, StopPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
  END_METHOD_RESOLVE()
};
