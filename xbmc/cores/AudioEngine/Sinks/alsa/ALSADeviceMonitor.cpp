/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ALSADeviceMonitor.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "utils/log.h"

#include "platform/linux/FDEventMonitor.h"

#include <libudev.h>

CALSADeviceMonitor::CALSADeviceMonitor() :
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

    const auto eventMonitor = CServiceBroker::GetPlatform().GetService<CFDEventMonitor>();
    eventMonitor->AddFD(CFDEventMonitor::MonitoredFD(udev_monitor_get_fd(m_udevMonitor), POLLIN,
                                                     FDEventCallback, m_udevMonitor),
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
    const auto eventMonitor = CServiceBroker::GetPlatform().GetService<CFDEventMonitor>();
    eventMonitor->RemoveFD(m_fdMonitorId);

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
      CLog::Log(LOGDEBUG, "CALSADeviceMonitor - ALSA card added (\"{}\", \"{}\")",
                udev_device_get_syspath(device), udev_device_get_devpath(device));
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
    CServiceBroker::GetActiveAE()->DeviceChange();
  }
}
