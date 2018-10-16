/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VTUtils.h"

#include "platform/MessagePrinter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <linux/major.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

using namespace KODI::WINDOWING::GBM;

bool CVTUtils::OpenTTY()
{
  m_ttyDevice = ttyname(STDIN_FILENO);

  m_fd.attach(open(m_ttyDevice.c_str(), O_RDWR | O_CLOEXEC));
  if (m_fd < 0)
  {
    CMessagePrinter::DisplayError(StringUtils::Format("ERROR: failed to open tty: %s",  m_ttyDevice));
    CLog::Log(LOGERROR, "CVTUtils::%s - failed to open tty: %s", __FUNCTION__, m_ttyDevice);
    return false;
  }

  struct stat buf;
  if (fstat(m_fd, &buf) == -1 || major(buf.st_rdev) != TTY_MAJOR || minor(buf.st_rdev) == 0)
  {
    CMessagePrinter::DisplayError(StringUtils::Format("ERROR: %s is not a vt",  m_ttyDevice));
    CLog::Log(LOGERROR, "CVTUtils::%s - %s is not a vt", __FUNCTION__, m_ttyDevice);
    return false;
  }

  int kdMode{-1};
  auto ret = ioctl(m_fd, KDGETMODE, &kdMode);
  if (ret)
  {
    CLog::Log(LOGERROR, "CVTUtils::%s - failed to get VT mode: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  if (kdMode != KD_TEXT)
  {
    CMessagePrinter::DisplayError(StringUtils::Format("ERROR: %s is already in graphics mode, is another display server running?", m_ttyDevice));
    CLog::Log(LOGERROR, "CVTUtils::%s - %s is already in graphics mode, is another display server running?", __FUNCTION__, m_ttyDevice);
    return false;
  }

  CLog::Log(LOGNOTICE, "CVTUtils::%s - opened tty: %s", __FUNCTION__, m_ttyDevice.c_str());

  return true;
}
