/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"
#include "GraphicBuffer.h"
class DllGraphicBufferInterface
{
public:
  virtual ~DllGraphicBufferInterface() = default;
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
