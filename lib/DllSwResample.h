#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __GNUC__
#pragma warning(disable:4244)
#endif
#if (defined USE_EXTERNAL_FFMPEG)
  #include <libswresample/swresample.h>
#else
  #include "libswresample/swresample.h"
#endif
}


#if (defined USE_EXTERNAL_FFMPEG) || (defined TARGET_DARWIN) 

// Use direct mapping
class DllSwResample : public DllDynamic
{
public:
  virtual ~DllSwResample() {}

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libswresample system library");
    return true;
  }
  virtual void Unload() {}
};

#else

class DllSwResample : public DllDynamic
{
  DECLARE_DLL_WRAPPER(DllSwResample, DLL_PATH_LIBSWRESAMPLE)

  LOAD_SYMBOLS()

  BEGIN_METHOD_RESOLVE()
  END_METHOD_RESOLVE()

  /* dependencies of libavformat */
  DllAvUtil m_dllAvUtil;

public:

  virtual bool Load()
  {
    if (!m_dllAvUtil.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif
