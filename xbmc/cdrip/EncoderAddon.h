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
  ssize_t Encode(uint8_t* pbtStream, size_t nNumBytesRead) override;
  bool Close() override;

  // Addon callback functions
  ssize_t Write(const uint8_t* data, size_t len) override;
  ssize_t Seek(ssize_t pos, int whence) override;

private:
  // Currently needed addon interface parts
  //@{
  static ssize_t cb_write(KODI_HANDLE kodiInstance, const uint8_t* data, size_t len);
  static ssize_t cb_seek(KODI_HANDLE kodiInstance, ssize_t pos, int whence);
  //@}
};

} /* namespace CDRIP */
} /* namespace KODI */
