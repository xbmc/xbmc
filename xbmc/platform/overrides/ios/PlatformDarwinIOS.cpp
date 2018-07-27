/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/darwin/PlatformDarwin.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformDarwin();
}
