/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include <alsa/asoundlib.h>

class CALSADeviceMonitor
{
public:
  CALSADeviceMonitor();
  ~CALSADeviceMonitor();

  void Start();
  void Stop();

private:
  static void FDEventCallback(int id, int fd, short revents, void *data);

  int m_fdMonitorId = 0;

  struct udev *m_udev;
  struct udev_monitor* m_udevMonitor;
};
