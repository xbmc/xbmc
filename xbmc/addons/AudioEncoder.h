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

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "cdrip/IEncoder.h"

namespace ADDON
{

  class CAudioEncoder : public IEncoder, public IAddonInstanceHandler
  {
  public:
    CAudioEncoder(BinaryAddonBasePtr addonBase);

    // Child functions related to IEncoder
    bool Init(AddonToKodiFuncTable_AudioEncoder& callbacks);
    int Encode(int nNumBytesRead, uint8_t* pbtStream);
    bool Close();

  private:
    AddonInstance_AudioEncoder m_struct;
  };

} /*namespace ADDON*/
