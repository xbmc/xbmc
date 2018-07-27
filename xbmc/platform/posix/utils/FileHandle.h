/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <unistd.h>

#include "utils/ScopeGuard.h"

namespace KODI
{
namespace UTILS
{
namespace POSIX
{

class CFileHandle : public CScopeGuard<int, -1, decltype(close)>
{
public:
  CFileHandle() noexcept : CScopeGuard(close, -1) {}
  explicit CFileHandle(int fd) : CScopeGuard(close, fd) {}
};

}
}
}
