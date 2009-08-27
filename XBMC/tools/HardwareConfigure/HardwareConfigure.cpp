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

#include "HardwareConfigure.h"
#include <libhal-storage.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

//#define HAL_HANDLEMOUNT

bool CHalManager::NewMessage;
DBusError CHalManager::m_Error;

/* A Removed device, It isn't possible to make a LibHalVolume from a removed device therefor
   we catch the UUID from the udi on the removal */
void CHalManager::DeviceRemoved(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  printf("HAL: Device (%s) Removed\n", udi);
//  g_HalManager.RemoveDevice(udi);
}

void CHalManager::DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  printf("HAL: Device (%s) gained capability %s\n", udi, capability);
  g_HalManager.ParseDevice(udi);
}

void CHalManager::DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability)
{
  NewMessage = true;
  printf("HAL: Device (%s) lost capability %s\n", udi, capability);
  g_HalManager.ParseDevice(udi);
}

/* HAL Property modified callback. If a device is mounted. This is called. */
void CHalManager::DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
  NewMessage = true;
  printf("HAL: Device (%s) Property %s modified\n", udi, key);
  g_HalManager.ParseDevice(udi);
}

void CHalManager::DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details)
{
  printf("HAL: Device (%s) Condition %s | %s\n", udi, condition_name, condition_details);
  NewMessage = true;
  g_HalManager.ParseDevice(udi);
}

/* HAL Device added. This is before mount. And here is the place to mount the volume in the future */
void CHalManager::DeviceAdded(LibHalContext *ctx, const char *udi)
{
  NewMessage = true;
  printf("HAL: Device (%s) Added\n", udi);
  g_HalManager.ParseDevice(udi);
}

CHalManager g_HalManager;

/* Iterate through all devices currently on the computer. Needed mostly at startup */
void CHalManager::GenerateGDL()
{
  if (m_Context == NULL)
    return;

  char **GDL;
  int i = 0;
  printf("HAL: Generating global device list\n");
  GDL = libhal_get_all_devices(g_HalManager.m_Context, &i, &m_Error);

  if (!ReadAvailableRemotes())
    return;

  for (i = 0; GDL[i]; i++)
  {
    if (ParseDevice(GDL[i]))
      break;
  }

  printf("HAL: Generated global device list, found %i\n", i);
}

CHalManager::CHalManager()
{
}

// Shutdown the connection and free the context
CHalManager::~CHalManager()
{
  if (m_Context != NULL)
    libhal_ctx_shutdown(m_Context, NULL);
  if (m_Context != NULL)
    libhal_ctx_free(m_Context);

  if (m_DBusSystemConnection != NULL)
  {
    dbus_connection_unref(m_DBusSystemConnection);
    m_DBusSystemConnection = NULL;
  }
  dbus_error_free(&m_Error); // Needed?
}

// Initialize
void CHalManager::Initialize(const char *LircConfPath)
{
  printf("HAL: Starting initializing\n");
  strcpy(m_LircConfPath, LircConfPath);

  g_HalManager.m_Context = g_HalManager.InitializeHal();
  if (g_HalManager.m_Context == NULL)
  {
    printf("HAL: no Hal context\n");
    return;
  }

  GenerateGDL();

  printf("HAL: Sucessfully initialized\n");
}

// Initialize basic DBus connection
bool CHalManager::InitializeDBus()
{
  if (m_DBusSystemConnection != NULL)
    return true;

  dbus_error_init (&m_Error);
  if (m_DBusSystemConnection == NULL && !(m_DBusSystemConnection = dbus_bus_get (DBUS_BUS_SYSTEM, &m_Error)))
  {
    printf("DBus: Could not get system bus: %s\n", m_Error.message);
    dbus_error_free (&m_Error);
  }

  if (m_DBusSystemConnection != NULL)
    return true;
  else
    return false;
}

// Initialize basic HAL connection
LibHalContext *CHalManager::InitializeHal()
{
  LibHalContext *ctx;
  char **devices;
  int nr;

  if (!InitializeDBus())
    return NULL;

  if (!(ctx = libhal_ctx_new()))
  {
    printf("HAL: failed to create a HAL context!\n");
    return NULL;
  }

  if (!libhal_ctx_set_dbus_connection(ctx, m_DBusSystemConnection))
    printf("HAL: Failed to connect with dbus\n");

  libhal_ctx_set_device_added(ctx, DeviceAdded);
  libhal_ctx_set_device_removed(ctx, DeviceRemoved);
  libhal_ctx_set_device_new_capability(ctx, DeviceNewCapability);
  libhal_ctx_set_device_lost_capability(ctx, DeviceLostCapability);
  libhal_ctx_set_device_property_modified(ctx, DevicePropertyModified);
  libhal_ctx_set_device_condition(ctx, DeviceCondition);

  if (!libhal_device_property_watch_all(ctx, &m_Error))
  {
    printf("HAL: Failed to set property watch %s\n", m_Error.message);
    dbus_error_free(&m_Error);
    libhal_ctx_free(ctx);
    return NULL;
  }

  if (!libhal_ctx_init(ctx, &m_Error))
  {
    printf("HAL: Failed to initialize hal context: %s\n", m_Error.message);
    dbus_error_free(&m_Error);
    libhal_ctx_free(ctx);
    return NULL;
  }

  /*
 * Do something to ping the HAL daemon - the above functions will
 * succeed even if hald is not running, so long as DBUS is.  But we
 * want to exit silently if hald is not running, to behave on
 * pre-2.6 systems.
 */
  if (!(devices = libhal_get_all_devices(ctx, &nr, &m_Error)))
  {
    printf("HAL: seems that Hal daemon is not running: %s\n", m_Error.message);
    dbus_error_free(&m_Error);

    libhal_ctx_shutdown(ctx, NULL);
    libhal_ctx_free(ctx);
    return NULL;
  }

  libhal_free_string_array(devices);

  return ctx;
}

// Called from ProcessSlow to trigger the callbacks from DBus
bool CHalManager::Update()
{
  if (m_Context == NULL)
    return false;

  if (!dbus_connection_read_write_dispatch(m_DBusSystemConnection, 0)) // We choose 0 that means we won't wait for a message
  {
    printf("DBus: System - read/write dispatch\n");
    return false;
  }
  if (NewMessage)
  {
    NewMessage = false;
    return true;
  }
  else
    return false;
}

/* Parse newly found device and add it to our remembered devices */
bool CHalManager::ParseDevice(const char *udi)
{
  int VendorID, ProductID;
  
  VendorID  = libhal_device_get_property_int(m_Context, udi, "usb.vendor_id", NULL);
  ProductID = libhal_device_get_property_int(m_Context, udi, "usb.product_id", NULL);

  const char *name = IsAllowedRemote(VendorID, ProductID);

  if (name != NULL)
  {
    printf("HAL: Found %s - %s\n", name, udi);
    if (MoveConfigs(name))
    {
      printf("HAL: Sucessfully created config for %s\n", name);
      return true;

      RunCommand(name);
    }
    else
      printf("HAL: Failed to create config for %s\n", name);
  }

  return false;
}

void Tokenize(const string& path, vector<string>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}

bool CHalManager::ReadAvailableRemotes()
{
  ifstream inputfile("AvailableRemotes");
  string line;

  m_AllowedRemotes.clear();

  if (inputfile.is_open())
  {
    while (!inputfile.eof())
    {
      getline(inputfile, line);
      if (line.size() > 0)
      {
        vector<string> tokens;
        Tokenize(line, tokens, " ");

        if (tokens[1].size() > 0 && tokens[0].size() > 0)
        {
          CHalDevice dev(atoi(tokens[1].c_str()), atoi(tokens[2].c_str()), tokens[0].c_str());
          printf("AvailableRemote: (%s) (%i) (%i)\n", dev.FriendlyName, dev.VendorID, dev.ProductID);
          m_AllowedRemotes.push_back(dev);
        }
      }
    }
    inputfile.close();

    return true;
  }
  return false;
}

const char *CHalManager::IsAllowedRemote(int VendorID, int ProductID)
{
  for (unsigned int i = 0; i < m_AllowedRemotes.size(); i++)
  {
    if (VendorID == m_AllowedRemotes[i].VendorID && ProductID == m_AllowedRemotes[i].ProductID)
      return m_AllowedRemotes[i].FriendlyName;
  }
  return NULL;
}

void CHalManager::RunCommand(const char *name)
{
  char script[1024];
  sprintf(script, "%s.sh", name);
  if (Exists(script))
  {
    popen(script, "r");
  }
}

bool CHalManager::MoveConfigs(const char *name)
{
  char lircd[1024], hardware[1024], lircdout[1024], hardwareout[1024];

  sprintf(lircd,        "%s.lircd.conf", name);
  sprintf(lircdout,     "%s/lircd.conf", m_LircConfPath);
  sprintf(hardware,     "%s.hardware.conf", name);
  sprintf(hardwareout,  "%s/hardware.conf", m_LircConfPath);

  bool temp = true;
  if (!Exists(lircd))
  {
    temp = false;
    printf("%s didn't exist\n", lircd);
  }
  if (!Exists(hardware))
  {
    temp = false;
    printf("%s didn't exist\n", hardware);
  }
  if (temp)
  {
    bool temp2 = true;
    printf("Copying %s -> %s\n", lircd, lircdout);
    if (!MoveConfig(lircd, lircdout))
    {
      printf("Failed to move %s -> %s\n", lircd, lircdout);
      temp2 = false;
    }
    printf("Copying %s -> %s\n", hardware, hardwareout);
    if (!MoveConfig(hardware, hardwareout))
    {
      printf("Failed to move %s -> %s\n", hardware, hardwareout);
      temp2 = false;
    }
    return temp2;
  }
  else
    return false;
}

bool CHalManager::Exists(const char *path)
{
  struct stat stFileInfo;

  if (stat(path, &stFileInfo) == 0)
    return true;
  else
    return false;
}

bool CHalManager::MoveConfig(const char *InputConfig, const char *OutputConfig)
{
  string line;

  ifstream inputfile(InputConfig);
  ofstream outputfile(OutputConfig, ios_base::out);

  if (inputfile.is_open() && outputfile.is_open())
  {
    while (!inputfile.eof())
    {
      getline(inputfile, line);
      outputfile << line <<"\n";
    }
    inputfile.close();
    outputfile.close();
    return true;
  }
  else
    return false;
}

int main(int argc, char* argv[])
{
  if (argc == 2)
    g_HalManager.Initialize(argv[1]);
  else
    printf("Usage: %s PathToLircConf\n", argv[0]);
}
