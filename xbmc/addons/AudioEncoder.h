/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "cdrip/IEncoder.h"

namespace ADDON
{

  class CAudioEncoder : public IEncoder, public IAddonInstanceHandler
  {
  public:
    explicit CAudioEncoder(BinaryAddonBasePtr addonBase);

    // Child functions related to IEncoder
    bool Init(AddonToKodiFuncTable_AudioEncoder& callbacks) override;
    int Encode(int nNumBytesRead, uint8_t* pbtStream) override;
    bool Close() override;

  private:
    AddonInstance_AudioEncoder m_struct;
  };

} /*namespace ADDON*/
