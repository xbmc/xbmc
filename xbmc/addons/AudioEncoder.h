/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_audioenc_types.h"
#include "cdrip/IEncoder.h"

namespace ADDON
{

  class CAudioEncoder : public CAddonDll, public IEncoder
  {
  public:
    CAudioEncoder(CAddonInfo addonInfo);
    virtual ~CAudioEncoder() {}

    // Things that MUST be supplied by the child classes
    bool Create();
    bool Init(audioenc_callbacks &callbacks);
    int Encode(int nNumBytesRead, uint8_t* pbtStream);
    bool Close();
    void Destroy();
    const std::string& Extension() { return m_extension; }

  private:
    std::string m_extension;
    void *m_context; ///< audio encoder context
    AUDIOENC_PROPS m_info;
    KodiToAddonFuncTable_AudioEncoder m_struct;
  };

} /*namespace ADDON*/
