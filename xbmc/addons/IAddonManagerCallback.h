/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <string>

namespace ADDON
{
/**
  * Class - IAddonMgrCallback
  * This callback should be inherited by any class which manages
  * specific addon types. Could be mostly used for Dll addon types to handle
  * cleanup before restart/removal
  */
class IAddonMgrCallback
{
public:
  virtual ~IAddonMgrCallback() = default;
  virtual bool RequestRestart(const std::string& addonId,
                              AddonInstanceId instanceId,
                              bool datachanged) = 0;
};

} /* namespace ADDON */
