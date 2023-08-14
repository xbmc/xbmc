/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon_base.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

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
      CAddonStatusHandler(const std::string& addonID,
                          AddonInstanceId instanceId,
                          ADDON_STATUS status,
                          bool sameThread = true);
      ~CAddonStatusHandler() override;

      /* Thread handling */
      void Process() override;
      void OnStartup() override;
      void OnExit() override;

    private:
      static CCriticalSection   m_critSection;
      const uint32_t m_instanceId;
      AddonPtr                  m_addon;
      ADDON_STATUS m_status = ADDON_STATUS_UNKNOWN;
  };
}
