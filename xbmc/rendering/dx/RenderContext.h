/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(TARGET_WINDOWS_DESKTOP)
#include "windowing/windows/WinSystemWin32DX.h"
#elif defined(TARGET_WINDOWS_STORE)
#include "windowing/win10/WinSystemWin10DX.h"
#endif
#include "ServiceBroker.h"

namespace DX
{
#if defined(TARGET_WINDOWS_DESKTOP)
  __inline CWinSystemWin32DX* Windowing()
  {
    return dynamic_cast<CWinSystemWin32DX*>(CServiceBroker::GetRenderSystem());
  }
#elif defined(TARGET_WINDOWS_STORE)
  __inline CWinSystemWin10DX* Windowing()
  {
    return dynamic_cast<CWinSystemWin10DX*>(CServiceBroker::GetRenderSystem());
  }
#endif
}
