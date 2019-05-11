/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDefs.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

DWORD GetLastError()
{
  return errno;
}

void SetLastError(DWORD dwErrCode)
{
  errno = dwErrCode;
}
