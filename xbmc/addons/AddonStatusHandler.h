/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAddon.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_types.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <string>

namespace ADDON
{
  /**
  * Class - CAddonStatusHandler
  * Used to inform the user about occurred errors and
  * changes inside Add-on's, and ask him what to do.
  * It can executed in the same thread as the calling
  * function or in a separate thread.
  */
  class CAddonStatusHandler : private CThread
  {
    public:
      CAddonStatusHandler(const std::string &addonID, ADDON_STATUS status, std::string message, bool sameThread = true);
      ~CAddonStatusHandler() override;

      /* Thread handling */
      void Process() override;
      void OnStartup() override;
      void OnExit() override;

    private:
      static CCriticalSection   m_critSection;
      AddonPtr                  m_addon;
      ADDON_STATUS              m_status;
      std::string               m_message;
  };


}
