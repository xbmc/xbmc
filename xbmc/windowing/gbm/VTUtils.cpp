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
  char ttyDevice[128];
  size_t ttyDeviceLength{128};

  auto ret = ttyname_r(STDIN_FILENO, ttyDevice, ttyDeviceLength);
  if (ret > 0)
  {
    std::string warningMessage;
    warningMessage.append(StringUtils::Format("ERROR: could not get a valid vt: %s\n", strerror(errno)));
    warningMessage.append("NOTICE: if using a systemd service make sure that both TTYpath=/dev/ttyN and StandardInput=tty are set\n");
    warningMessage.append("NOTICE: https://www.freedesktop.org/software/systemd/man/systemd.exec.html#Logging%20and%20Standard%20Input/Output");
    CMessagePrinter::DisplayError(warningMessage);

    CLog::Log(LOGWARNING, "CVTUtils::%s -  could not get a valid vt", __FUNCTION__);
    return false;
  }

  m_ttyDevice = ttyDevice;

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
  ret = ioctl(m_fd, KDGETMODE, &kdMode);
  if (ret < 0)
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
