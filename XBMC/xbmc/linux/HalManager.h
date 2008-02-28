/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef __APPLE
#ifndef HALMANAGER_H
#define HALMANAGER_H
#endif

#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <libhal.h>
#include "LinuxFileSystem.h"
#include <vector>


#define BYTE char
#include "../utils/log.h"

class CHalManager
{
public:
bool Update();

static std::vector<CDevice> GetDevices();
void Initialize();
CHalManager();
~CHalManager();

protected:
DBusConnection *m_DBusConnection;
LibHalContext  *m_Context;
static DBusError m_Error;
static bool NewMessage;

private:
LibHalContext *InitializeHal();
bool InitializeDBus();

static bool DeviceFromVolumeUdi(const char *udi, CDevice *device);
static std::vector<CDevice> DeviceFromDriveUdi(const char *udi);

//Callbacks DBus
static DBusHandlerResult DBusFilter(DBusConnection *connection, DBusMessage *message, void *user_data);
//Callbacks HAL
static void DeviceRemoved(LibHalContext *ctx, const char *udi);
static void DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability);
static void DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability);
static void DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added);
static void DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details);
static void DeviceAdded(LibHalContext *ctx, const char *udi);
};

extern CHalManager g_HalManager;
#endif //__APPLE__
