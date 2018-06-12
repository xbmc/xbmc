/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#if defined(TARGET_WINDOWS_DESKTOP)
#include "windowing/windows/WinSystemWin32DX.h"
#elif defined(TARGET_WINDOWS_STORE)
#include "windowing/win10/WinSystemWin10DX.h"
#endif
#include "xbmc/ServiceBroker.h"

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
