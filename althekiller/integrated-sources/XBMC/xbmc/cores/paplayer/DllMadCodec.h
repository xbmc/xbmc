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
#include "dec_if.h"

class DllMadCodecInterface
{
public:
  virtual ~DllMadCodecInterface() {}
  virtual IAudioDecoder* CreateAudioDecoder (unsigned int type, IAudioOutput **output)=0;
};

class DllMadCodec : public DllDynamic, DllMadCodecInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllMadCodec, Q:\\system\\players\\PAPlayer\\MADCodec.dll)
#elif defined(__APPLE__)
  DECLARE_DLL_WRAPPER(DllMadCodec,
                      Q:\\system\\players\\paplayer\\MADCodec-osx.so)
#elif defined(__x86_64__)
  DECLARE_DLL_WRAPPER(DllMadCodec,
                      Q:\\system\\players\\paplayer\\MADCodec-x86_64-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllMadCodec,
                      Q:\\system\\players\\paplayer\\MADCodec-i486-linux.so)
#endif /* _LINUX */
  DEFINE_METHOD2(IAudioDecoder*, CreateAudioDecoder, (unsigned int p1, IAudioOutput **p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(CreateAudioDecoder)
  END_METHOD_RESOLVE()
};
