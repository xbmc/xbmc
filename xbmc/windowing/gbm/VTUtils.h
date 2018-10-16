/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/utils/FileHandle.h"

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVTUtils
{
public:
  CVTUtils() = default;
  ~CVTUtils() = default;

  bool OpenTTY();

private:
  KODI::UTILS::POSIX::CFileHandle m_fd;
  std::string m_ttyDevice;
};

}
}
}
