/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDevProvider.h"

#include "platform/posix/PosixMountProvider.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

extern "C" {
#include <libudev.h>
#include <poll.h>
}

namespace
{
const char* get_mountpoint(const char* devnode)
{
  static char buf[4096];
  const char *delim = " ";
  const char *mountpoint = nullptr;
  FILE *fp = fopen("/proc/mounts", "r");
  if (!fp)
    return nullptr;

  while (fgets(buf, sizeof (buf), fp))
  {
    const char *node = strtok(buf, delim);
    if (strcmp(node, devnode) == 0)
    {
      mountpoint = strtok(nullptr, delim);
      break;
    }
  }

  if (mountpoint)
  {
    // If mount point contain characters like space, it is converted to
    // "\040". This situation should be handled.
    char *c1, *c2;
    for (c1 = c2 = const_cast<char*>(mountpoint); *c2; ++c1)
    {
      if (*c2 == '\\')
      {
        *c1 = (((c2[1] - '0') << 6) | ((c2[2] - '0') << 3) | (c2[3] - '0'));
        c2 += 4;
        continue;
      }
      if (c1 != c2)
        *c1 = *c2;
      ++c2;
    }
    *c1 = *c2;
  }

  fclose(fp);
  return mountpoint;
}
} // namespace

CUDevProvider::CUDevProvider()
{
  m_udev = nullptr;
  m_udevMon = nullptr;
}

void CUDevProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected UDev as storage provider");

  m_udev = udev_new();
  if (!m_udev)
  {
    CLog::Log(LOGERROR, "{} - failed to allocate udev context", __FUNCTION__);
    return;
  }
  /* set up a devices monitor that listen for any device change */
  m_udevMon = udev_monitor_new_from_netlink(m_udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(m_udevMon, "block", "disk");
  udev_monitor_filter_add_match_subsystem_devtype(m_udevMon, "block", "partition");
  udev_monitor_enable_receiving(m_udevMon);

  PumpDriveChangeEvents(nullptr);
}

void CUDevProvider::Stop()
{
  udev_monitor_unref(m_udevMon);
  udev_unref(m_udev);
}

void CUDevProvider::GetDisks(std::vector<CMediaSource>& disks, bool removable)
{
  // enumerate existing block devices
  struct udev_enumerate *u_enum = udev_enumerate_new(m_udev);
  if (!u_enum)
  {
    fprintf(stderr, "Error: udev_enumerate_new(udev)\n");
    return;
  }

  udev_enumerate_add_match_subsystem(u_enum, "block");
  udev_enumerate_add_match_property(u_enum, "DEVTYPE", "disk");
  udev_enumerate_add_match_property(u_enum, "DEVTYPE", "partition");
  udev_enumerate_scan_devices(u_enum);

  struct udev_list_entry *u_list_ent;
  struct udev_list_entry *u_first_list_ent;
  u_first_list_ent = udev_enumerate_get_list_entry(u_enum);
  udev_list_entry_foreach(u_list_ent, u_first_list_ent)
  {
    const char *name = udev_list_entry_get_name(u_list_ent);
    struct udev *context = udev_enumerate_get_udev(u_enum);
    struct udev_device *device = udev_device_new_from_syspath(context, name);
    if (!device)
      continue;

    // filter out devices without devnode
    const char* devnode = udev_device_get_devnode(device);
    if (!devnode)
    {
      udev_device_unref(device);
      continue;
    }

    // filter out devices that are not mounted
    const char* mountpoint = get_mountpoint(devnode);
    if (!mountpoint)
    {
      udev_device_unref(device);
      continue;
    }

    // filter out root partition
    if (strcmp(mountpoint, "/") == 0)
    {
      udev_device_unref(device);
      continue;
    }

    // filter out things mounted on /tmp
    if (strstr(mountpoint, "/tmp"))
    {
      udev_device_unref(device);
      continue;
    }

    // look for devices on the usb bus, or mounted on */media/ (sdcards), or optical devices
    const char *bus = udev_device_get_property_value(device, "ID_BUS");
    const char *optical = udev_device_get_property_value(device, "ID_CDROM"); // matches also DVD, Blu-ray
    bool isRemovable = ((bus        && strstr(bus, "usb")) ||
                        (optical    && strstr(optical,"1"))  ||
                        (mountpoint && strstr(mountpoint, "/media/")));

    // filter according to requested device type
    if (removable != isRemovable)
    {
      udev_device_unref(device);
      continue;
    }

    const char *udev_label = udev_device_get_property_value(device, "ID_FS_LABEL");
    std::string label;
    if (udev_label)
      label = udev_label;
    else
      label = URIUtils::GetFileName(mountpoint);

    CMediaSource share;
    share.strName  = label;
    share.strPath  = mountpoint;
    share.m_ignore = true;
    if (isRemovable)
    {
      if (optical)
        share.m_iDriveType = SourceType::OPTICAL_DISC;
      else
        share.m_iDriveType = SourceType::REMOVABLE;
    }
    else
      share.m_iDriveType = SourceType::LOCAL;

    disks.push_back(share);
    udev_device_unref(device);
  }
  udev_enumerate_unref(u_enum);
}

void CUDevProvider::GetLocalDrives(std::vector<CMediaSource>& localDrives)
{
  GetDisks(localDrives, false);
}

void CUDevProvider::GetRemovableDrives(std::vector<CMediaSource>& removableDrives)
{
  GetDisks(removableDrives, true);
}

bool CUDevProvider::Eject(const std::string& mountpath)
{
  // just go ahead and try to umount the disk
  // if it does umount, life is good, if not, no loss.
  std::string cmd = "umount \"" + mountpath + "\"";
  int status = system(cmd.c_str());

  if (status == 0)
    return true;

  return false;
}

std::vector<std::string> CUDevProvider::GetDiskUsage()
{
  CPosixMountProvider legacy;
  return legacy.GetDiskUsage();
}

bool CUDevProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool changed = false;

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(udev_monitor_get_fd(m_udevMon), &readfds);

  // non-blocking, check the file descriptor for received data
  struct timeval tv = {};
  int count = select(udev_monitor_get_fd(m_udevMon) + 1, &readfds, nullptr, nullptr, &tv);
  if (count < 0)
    return false;

  if (FD_ISSET(udev_monitor_get_fd(m_udevMon), &readfds))
  {
    struct udev_device* dev = udev_monitor_receive_device(m_udevMon);
    if (!dev)
      return false;

    const char* action = udev_device_get_action(dev);
    const char* devnode = udev_device_get_devnode(dev);
    if (action && devnode)
    {
      MEDIA_DETECT::STORAGE::StorageDevice storageDevice;
      const char *udev_label = udev_device_get_property_value(dev, "ID_FS_LABEL");
      const char* mountpoint = get_mountpoint(devnode);
      if (udev_label)
        storageDevice.label = udev_label;
      else if (mountpoint)
      {
        storageDevice.label = URIUtils::GetFileName(mountpoint);
        storageDevice.path = mountpoint;
      }

      const char *fs_usage = udev_device_get_property_value(dev, "ID_FS_USAGE");
      if (mountpoint && strcmp(action, "add") == 0 && (fs_usage && strcmp(fs_usage, "filesystem") == 0))
      {
        CLog::Log(LOGINFO, "UDev: Added {}", mountpoint);
        if (callback)
          callback->OnStorageAdded(storageDevice);
        changed = true;
      }
      if (strcmp(action, "remove") == 0 && (fs_usage && strcmp(fs_usage, "filesystem") == 0))
      {
        if (callback)
          callback->OnStorageSafelyRemoved(storageDevice);
        changed = true;
      }
      // browse disk dialog is not wanted for blu-rays
      const char *bd = udev_device_get_property_value(dev, "ID_CDROM_MEDIA_BD");
      if (strcmp(action, "change") == 0 && !(bd && strcmp(bd, "1") == 0))
      {
        const char *optical = udev_device_get_property_value(dev, "ID_CDROM");
        const bool isOptical = optical && (strcmp(optical, "1") == 0);
        storageDevice.type =
            isOptical ? MEDIA_DETECT::STORAGE::Type::OPTICAL : MEDIA_DETECT::STORAGE::Type::UNKNOWN;
        storageDevice.path = devnode;

        if (mountpoint && isOptical)
        {
          CLog::Log(LOGINFO, "UDev: Changed / Added {}", mountpoint);
          if (callback)
            callback->OnStorageAdded(storageDevice);
          changed = true;
        }
        const char *eject_request = udev_device_get_property_value(dev, "DISK_EJECT_REQUEST");
        if (eject_request && strcmp(eject_request, "1") == 0)
        {
          if (callback)
            callback->OnStorageSafelyRemoved(storageDevice);
          changed = true;
        }
      }
    }
    udev_device_unref(dev);
  }

  return changed;
}
