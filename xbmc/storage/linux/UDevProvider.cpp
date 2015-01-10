/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "UDevProvider.h"

#ifdef HAVE_LIBUDEV

#include "linux/PosixMountProvider.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

extern "C" {
#include <libudev.h>
#include <poll.h>
}

static const char *get_mountpoint(const char *devnode)
{
  static char buf[4096];
  const char *delim = " ";
  const char *mountpoint = NULL;
  FILE *fp = fopen("/proc/mounts", "r");

  while (fgets(buf, sizeof (buf), fp))
  {
    const char *node = strtok(buf, delim);
    if (strcmp(node, devnode) == 0)
    {
      mountpoint = strtok(NULL, delim);
      break;
    }
  }

  if (mountpoint != NULL)
  {
    // If mount point contain characters like space, it is converted to
    // "\040". This situation should be handled.
    char *c1, *c2;
    for (c1 = c2 = (char*)mountpoint; *c2; ++c1)
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

CUDevProvider::CUDevProvider()
{
  m_udev    = NULL;
  m_udevMon = NULL;
}

void CUDevProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected UDev as storage provider");

  m_udev = udev_new();
  if (!m_udev)
  {
    CLog::Log(LOGERROR, "%s - failed to allocate udev context", __FUNCTION__);
    return;
  }
  /* set up a devices monitor that listen for any device change */
  m_udevMon = udev_monitor_new_from_netlink(m_udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(m_udevMon, "block", "disk");
  udev_monitor_filter_add_match_subsystem_devtype(m_udevMon, "block", "partition");
  udev_monitor_enable_receiving(m_udevMon);

  PumpDriveChangeEvents(NULL);
}

void CUDevProvider::Stop()
{
  udev_monitor_unref(m_udevMon);
  udev_unref(m_udev);
}

void CUDevProvider::GetDisks(VECSOURCES& disks, bool removable)
{
  // enumerate existing block devices
  struct udev_enumerate *u_enum = udev_enumerate_new(m_udev);
  if (u_enum == NULL)
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
    if (device == NULL)
      continue;

    // filter out devices that are not mounted
    const char *mountpoint = get_mountpoint(udev_device_get_devnode(device));
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
        share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
      else
        share.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
    }
    else
      share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;

    disks.push_back(share);
    udev_device_unref(device);
  }
  udev_enumerate_unref(u_enum);
}

void CUDevProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  GetDisks(localDrives, false);
}

void CUDevProvider::GetRemovableDrives(VECSOURCES &removableDrives)
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
  struct timeval tv = {0};
  int count = select(udev_monitor_get_fd(m_udevMon) + 1, &readfds, NULL, NULL, &tv);
  if (count < 0)
    return false;

  if (FD_ISSET(udev_monitor_get_fd(m_udevMon), &readfds))
  {
		struct udev_device *dev = udev_monitor_receive_device(m_udevMon);
    if (!dev)
      return false;

    const char *action  = udev_device_get_action(dev);
    const char *devtype = udev_device_get_devtype(dev);
    if (action)
    {
      std::string label;
      const char *udev_label = udev_device_get_property_value(dev, "ID_FS_LABEL");
      const char *mountpoint = get_mountpoint(udev_device_get_devnode(dev));
      if (udev_label)
        label = udev_label;
      else if (mountpoint)
        label = URIUtils::GetFileName(mountpoint);

      if (!strcmp(action, "add") && !strcmp(devtype, "partition"))
      {
        CLog::Log(LOGNOTICE, "UDev: Added %s", mountpoint);
        if (callback)
          callback->OnStorageAdded(label, mountpoint);
        changed = true;
      }
      if (!strcmp(action, "remove") && !strcmp(devtype, "partition"))
      {
        CLog::Log(LOGNOTICE, "UDev: Removed %s", mountpoint);
        if (callback)
          callback->OnStorageSafelyRemoved(label);
        changed = true;
      }
    }
    udev_device_unref(dev);
  }

  return changed;
}

#endif
