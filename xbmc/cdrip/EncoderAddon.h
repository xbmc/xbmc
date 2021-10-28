/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Encoder.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/AudioEncoder.h"

namespace KODI
{
namespace CDRIP
{

class CEncoderAddon : public CEncoder, public ADDON::IAddonInstanceHandler
{
public:
  explicit CEncoderAddon(const ADDON::AddonInfoPtr& addonInfo);
  ~CEncoderAddon() override;

  // Child functions related to IEncoder within CEncoder
  bool Init() override;
  int Encode(int nNumBytesRead, uint8_t* pbtStream) override;
  bool Close() override;

  // Addon callback functions
  int Write(const uint8_t* data, int len) override;
  int64_t Seek(int64_t pos, int whence) override;

private:
  // Currently needed addon interface parts
  //@{
  static int cb_write(KODI_HANDLE kodiInstance, const uint8_t* data, int len);
  static int64_t cb_seek(KODI_HANDLE kodiInstance, int64_t pos, int whence);
  AddonInstance_AudioEncoder m_struct;
  //@}

  KODI_HANDLE m_addonInstance;
};

} /* namespace CDRIP */
} /* namespace KODI */
