/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWin32.h"

#include "windowing/windows/WinSystemWin32DX.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin32();
}

void CPlatformWin32::Init()
{
  CWinSystemWin32DX::Register();
}
