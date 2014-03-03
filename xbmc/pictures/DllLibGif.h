#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <gif_lib.h>
#include "DynamicDll.h"

class DllLibGifInterface
{
public:
    virtual ~DllLibGifInterface() {}
    virtual GifFileType* DGifOpenFileName(const char *GifFileName, int *Error)=0;
    virtual int DGifCloseFile(GifFileType* GifFile)=0;    
    virtual int DGifSlurp(GifFileType* GifFile)=0;
    virtual int DGifSavedExtensionToGCB(GifFileType *GifFile, int ImageIndex, GraphicsControlBlock *GCB)=0;
};

class DllLibGif : public DllDynamic, DllLibGifInterface
{
  DECLARE_DLL_WRAPPER(DllLibGif, DLL_PATH_LIBGIF)
  DEFINE_METHOD1(char*, GifErrorString, (int p1))
  DEFINE_METHOD2(GifFileType*, DGifOpenFileName, (const char *p1, int *p2))
  DEFINE_METHOD1(int, DGifCloseFile, (GifFileType* p1))
  DEFINE_METHOD1(int, DGifSlurp, (GifFileType* p1))
  DEFINE_METHOD3(int, DGifSavedExtensionToGCB, (GifFileType *p1, int p2, GraphicsControlBlock *p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(GifErrorString)
    RESOLVE_METHOD(DGifOpenFileName)
    RESOLVE_METHOD(DGifCloseFile)
    RESOLVE_METHOD(DGifSlurp)
    RESOLVE_METHOD(DGifSavedExtensionToGCB)
  END_METHOD_RESOLVE()
};

