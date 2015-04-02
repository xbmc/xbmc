/*
 *      Copyright (C) 2014 Team Kodi
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#if defined(HAS_ALSA) && defined(HAVE_LIBUDEV)

#include <libudev.h>

#include "ALSADeviceMonitor.h"
#include "AEFactory.h"
#include "linux/FDEventMonitor.h"
#include "utils/log.h"

CALSADeviceMonitor::CALSADeviceMonitor() :
  m_fdMonitorId(0),
  m_udev(NULL),
  m_udevMonitor(NULL)
{
}

CALSADeviceMonitor::~CALSADeviceMonitor()
{
  Stop();
}

void CALSADeviceMonitor::Start()
{
  int err;

  if (!m_udev)
  {
    m_udev = udev_new();
    if (!m_udev)
    {
      CLog::Log(LOGWARNING, "CALSADeviceMonitor::Start - Unable to open udev handle");
      return;
    }

    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor)
    {
      CLog::Log(LOGERROR, "CALSADeviceMonitor::Start - udev_monitor_new_from_netlink() failed");
      goto err_unref_udev;
    }

    err = udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "sound", NULL);
    if (err)
    {
      CLog::Log(LOGERROR, "CALSADeviceMonitor::Start - udev_monitor_filter_add_match_subsystem_devtype() failed");
      goto err_unref_monitor;
    }

    err = udev_monitor_enable_receiving(m_udevMonitor);
    if (err)
    {
      CLog::Log(LOGERROR, "CALSADeviceMonitor::Start - udev_monitor_enable_receiving() failed");
      goto err_unref_monitor;
    }

    g_fdEventMonitor.AddFD(
        CFDEventMonitor::MonitoredFD(udev_monitor_get_fd(m_udevMonitor),
                                     POLLIN, FDEventCallback, m_udevMonitor),
        m_fdMonitorId);
  }

  return;

err_unref_monitor:
  udev_monitor_unref(m_udevMonitor);
  m_udevMonitor = NULL;
err_unref_udev:
  udev_unref(m_udev);
  m_udev = NULL;
}

void CALSADeviceMonitor::Stop()
{
  if (m_udev)
  {
    g_fdEventMonitor.RemoveFD(m_fdMonitorId);

    udev_monitor_unref(m_udevMonitor);
    m_udevMonitor = NULL;
    udev_unref(m_udev);
    m_udev = NULL;
  }
}

void CALSADeviceMonitor::FDEventCallback(int id, int fd, short revents, void *data)
{
  struct udev_monitor *udevMonitor = (struct udev_monitor *)data;
  bool audioDevicesChanged = false;
  struct udev_device *device;

  while ((device = udev_monitor_receive_device(udevMonitor)) != NULL)
  {
    const char* action = udev_device_get_action(device);
    const char* soundInitialized = udev_device_get_property_value(device, "SOUND_INITIALIZED");

    if (!action || !soundInitialized)
      continue;

    /* cardX devices emit a "change" event when ready (i.e. all subdevices added) */
    if (strcmp(action, "change") == 0)
    {
      CLog::Log(LOGDEBUG, "CALSADeviceMonitor - ALSA card added (\"%s\", \"%s\")", udev_device_get_syspath(device), udev_device_get_devpath(device));
      audioDevicesChanged = true;
    }
    else if (strcmp(action, "remove") == 0)
    {
      CLog::Log(LOGDEBUG, "CALSADeviceMonitor - ALSA card removed");
      audioDevicesChanged = true;
    }

    udev_device_unref(device);
  }

  if (audioDevicesChanged)
  {
    CAEFactory::DeviceChange();
  }
}

#endif
