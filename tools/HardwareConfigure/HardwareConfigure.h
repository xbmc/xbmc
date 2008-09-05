#ifndef HARDWARECONFIGURE_H
#define HARDWARECONFIGURE_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <libhal.h>
#include <vector>

using namespace std;

class CHalDevice
{
public:
  int ProductID;
  int VendorID;
  char FriendlyName[1024];
  CHalDevice(int vendorID, int productID, const char *friendlyName) { ProductID = productID; VendorID = vendorID; strcpy(FriendlyName, friendlyName); }
};

class CHalManager
{
public:
  bool Update();

  void Initialize(const char *LircConfPath);
  CHalManager();
  ~CHalManager();
protected:
  DBusConnection *m_DBusSystemConnection;
  LibHalContext  *m_Context;
  static DBusError m_Error;
  static bool NewMessage;

  bool ParseDevice(const char *udi);
private:
  char m_LircConfPath[1024];
  vector<CHalDevice> m_AllowedRemotes;

  LibHalContext *InitializeHal();
  bool InitializeDBus();
  void GenerateGDL();
  bool MoveConfigs(const char *udi);
  bool MoveConfig(const char *InputConfig, const char *OutputConfig);
  bool Exists(const char *path);
  const char *IsAllowedRemote(int VendorID, int ProductID);
  bool ReadAvailableRemotes();
  void RunCommand(const char *name);

  //Callbacks HAL
  static void DeviceRemoved(LibHalContext *ctx, const char *udi);
  static void DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability);
  static void DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability);
  static void DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added);
  static void DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details);
  static void DeviceAdded(LibHalContext *ctx, const char *udi);
};

extern CHalManager g_HalManager;
#endif
