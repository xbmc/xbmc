/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PosixInterfaceForCLog.h"

#if !defined(TARGET_ANDROID) && !defined(TARGET_DARWIN)
std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CPosixInterfaceForCLog>();
}
#endif
