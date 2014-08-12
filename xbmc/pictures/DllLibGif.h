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
#ifndef CONTINUE_EXT_FUNC_CODE
#define CONTINUE_EXT_FUNC_CODE 0
#endif

#ifndef DISPOSAL_UNSPECIFIED
#define DISPOSAL_UNSPECIFIED 0
#endif

#ifndef DISPOSE_DO_NOT
#define DISPOSE_DO_NOT 1
#endif

#ifndef DISPOSE_BACKGROUND
#define DISPOSE_BACKGROUND 2
#endif

#ifndef DISPOSE_PREVIOUS
#define DISPOSE_PREVIOUS 3
#endif

class DllLibGifInterface
{
public:
    virtual ~DllLibGifInterface() {}
#if GIFLIB_MAJOR == 5
    virtual const char* GifErrorString(int ErrorCode) = 0;
    virtual GifFileType* DGifOpenFileName(const char *GifFileName, int *Error) = 0;
    virtual GifFileType *DGifOpen(void *userPtr, InputFunc readFunc, int *Error) = 0;
    virtual int DGifSavedExtensionToGCB(GifFileType *GifFile, int ImageIndex, GraphicsControlBlock *GCB) = 0;
    virtual int DGifCloseFile(GifFileType* GifFile, int *Error)=0;
#else
    virtual char* GifErrorString() = 0;
    virtual GifFileType* DGifOpenFileName(const char *GifFileName) = 0;
    virtual GifFileType *DGifOpen(void *userPtr, InputFunc readFunc)=0;
    virtual int DGifGetExtension(GifFileType * GifFile, int *GifExtCode, GifByteType ** GifExtension) = 0;
    virtual int DGifGetExtensionNext(GifFileType * GifFile, GifByteType ** GifExtension) = 0;
    virtual int DGifCloseFile(GifFileType* GifFile)=0;
#endif
    virtual int DGifSlurp(GifFileType* GifFile)=0;
};

class DllLibGif : public DllDynamic, DllLibGifInterface
{
  DECLARE_DLL_WRAPPER(DllLibGif, DLL_PATH_LIBGIF)

#if GIFLIB_MAJOR == 5
  DEFINE_METHOD1(const char*, GifErrorString, (int p1))
  DEFINE_METHOD2(GifFileType*, DGifOpenFileName, (const char *p1, int *p2))
  DEFINE_METHOD3(GifFileType*, DGifOpen, (void *p1, InputFunc p2, int *p3))
  DEFINE_METHOD3(int, DGifSavedExtensionToGCB, (GifFileType *p1, int p2, GraphicsControlBlock *p3))
  DEFINE_METHOD2(int, DGifCloseFile, (GifFileType* p1, int *p2))
#else
  DEFINE_METHOD0(char*, GifErrorString)
  DEFINE_METHOD1(GifFileType*, DGifOpenFileName, (const char *p1))
  DEFINE_METHOD2(GifFileType*, DGifOpen, (void *p1, InputFunc p2))
  DEFINE_METHOD3(int, DGifGetExtension, (GifFileType *p1, int *p2, GifByteType **p3))
  DEFINE_METHOD2(int, DGifGetExtensionNext, (GifFileType *p1, GifByteType **p2))
  DEFINE_METHOD1(int, DGifCloseFile, (GifFileType* p1))
#endif
  DEFINE_METHOD1(int, DGifSlurp, (GifFileType* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(GifErrorString)
    RESOLVE_METHOD(DGifOpenFileName)
    RESOLVE_METHOD(DGifOpen)
    RESOLVE_METHOD(DGifCloseFile)
    RESOLVE_METHOD(DGifSlurp)
#if GIFLIB_MAJOR == 4
  RESOLVE_METHOD(DGifGetExtension)
  RESOLVE_METHOD(DGifGetExtensionNext)
#else
    RESOLVE_METHOD(DGifSavedExtensionToGCB)
#endif
  END_METHOD_RESOLVE()
};

