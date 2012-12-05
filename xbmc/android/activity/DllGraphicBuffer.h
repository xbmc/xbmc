#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "GraphicBuffer.h"
class DllGraphicBufferInterface
{
public:
  virtual ~DllGraphicBufferInterface() {}
  virtual void    GraphicBufferCtor(void *, uint32_t w, uint32_t h, uint32_t format, gfxImageUsage usage)=0;
  virtual void    GraphicBufferDtor(void *)=0;
  virtual int     GraphicBufferLock(void*, uint32_t usage, void **addr)=0;
  virtual int     GraphicBufferUnlock(void*)=0;
  virtual int     GraphicBufferGetNativeBuffer(void*)=0;
};

class DllGraphicBuffer : public DllDynamic, DllGraphicBufferInterface
{
  DECLARE_DLL_WRAPPER(DllGraphicBuffer, DLL_PATH_LIBUI)

  DEFINE_METHOD5(void, GraphicBufferCtor, (void *p1, uint32_t p2, uint32_t p3, uint32_t p4, gfxImageUsage p5))
  DEFINE_METHOD1(void, GraphicBufferDtor, (void* p1))
  DEFINE_METHOD3(int, GraphicBufferLock,  (void *p1, uint32_t p2, void** p3))
  DEFINE_METHOD1(int, GraphicBufferUnlock,(void* p1))
  DEFINE_METHOD1(int, GraphicBufferGetNativeBuffer,(void* p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(_ZN7android13GraphicBufferC1Ejjij, GraphicBufferCtor)
    RESOLVE_METHOD_RENAME(_ZN7android13GraphicBufferD1Ev, GraphicBufferDtor)
    RESOLVE_METHOD_RENAME(_ZN7android13GraphicBuffer4lockEjPPv, GraphicBufferLock)
    RESOLVE_METHOD_RENAME(_ZN7android13GraphicBuffer6unlockEv, GraphicBufferUnlock)
    RESOLVE_METHOD_RENAME(_ZNK7android13GraphicBuffer15getNativeBufferEv, GraphicBufferGetNativeBuffer)
  END_METHOD_RESOLVE()
};