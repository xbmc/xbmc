/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "DynamicDll.h"

extern "C" {

}

class DllLibSSE4Interface
{
public:
  virtual ~DllLibSSE4Interface() = default;
  virtual void copy_frame(void * pSrc, void * pDest, void * pCacheBlock, unsigned int width, unsigned int height, unsigned int pitch) = 0;
};

class DllLibSSE4 : public DllDynamic, DllLibSSE4Interface
{
  DECLARE_DLL_WRAPPER(DllLibSSE4, DLL_PATH_LIBSSE4)
  DEFINE_METHOD6(void, copy_frame, (void *p1, void *p2, void *p3, unsigned int p4, unsigned int p5, unsigned int p6))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(copy_frame)
  END_METHOD_RESOLVE()
};
