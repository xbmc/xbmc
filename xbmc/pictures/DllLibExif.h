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

#include "DynamicDll.h"
#include "lib/libexif/libexif.h"

class DllLibExifInterface
{
public:
    virtual bool process_jpeg(const char *, ExifInfo_t *, IPTCInfo_t *)=0;
    virtual ~DllLibExifInterface() {}
};

class DllLibExif : public DllDynamic, DllLibExifInterface
{
  DECLARE_DLL_WRAPPER(DllLibExif, DLL_PATH_LIBEXIF)
  DEFINE_METHOD3(bool, process_jpeg, (const char *p1, ExifInfo_t *p2, IPTCInfo_t *p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(process_jpeg)
  END_METHOD_RESOLVE()
};
