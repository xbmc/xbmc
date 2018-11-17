/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SessionUtils.h"

#if defined(HAS_DBUS)
#include "platform/linux/LogindUtils.h"
#endif

#include <fcntl.h>
#include <unistd.h>

std::shared_ptr<CSessionUtils> CSessionUtils::GetSession()
{
#if defined(HAS_DBUS)
  return std::make_shared<CLogindUtils>();
#endif
  return std::make_shared<CSessionUtils>();
}

int CSessionUtils::Open(const std::string& path, int flags)
{
  return open(path.c_str(), O_RDWR | flags);
}

void CSessionUtils::Close(int fd)
{
  close(fd);
}
