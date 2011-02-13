#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "AddonDll.h"
#include "include/xbmc_ac_types.h"

class CCriticalSection;

typedef DllAddon<AudioCodec, AC_PROPS> DllAudioCodec;

namespace ADDON
{
  class CAudioCodec : public CAddonDll<DllAudioCodec, AudioCodec, AC_PROPS>
  {
    public:
      CAudioCodec(const ADDON::AddonProps &props) :
        CAddonDll<DllAudioCodec, AudioCodec, AC_PROPS>(props)
      {
      }

      CAudioCodec(const cp_extension_t *ext);

      bool Supports(const CStdString& ext);
      CStdString GetExtensions();

      AC_INFO* Init(const char* strFile);
      void DeInit(AC_INFO* strFile);
      int64_t Seek(AC_INFO* info, int64_t seektime);
      int ReadPCM(AC_INFO* info, void* pBuffer, unsigned int size,
                  unsigned int* actualsize);
    protected:
      CStdString m_exts;
  };
  typedef boost::shared_ptr<CAudioCodec> AudioCodecPtr;
}
