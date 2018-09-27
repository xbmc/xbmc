/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include <algorithm>
#include <functional>

#include "PosixMountProvider.h"
#include "guilib/LocalizeStrings.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "UDisks2Provider.h"

#define BOOL2SZ(b) ((b) ? "true" : "false")

#define DBUS_INTERFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"

#define UDISKS2_SERVICE_UDISKS2 "org.freedesktop.UDisks2"

#define UDISKS2_PATH_MANAGER "/org/freedesktop/UDisks2/Manager"
#define UDISKS2_PATH_UDISKS2 "/org/freedesktop/UDisks2"

#define UDISKS2_INTERFACE_BLOCK "org.freedesktop.UDisks2.Block"
#define UDISKS2_INTERFACE_DRIVE "org.freedesktop.UDisks2.Drive"
#define UDISKS2_INTERFACE_FILESYSTEM "org.freedesktop.UDisks2.Filesystem"
#define UDISKS2_INTERFACE_MANAGER "org.freedesktop.UDisks2.Manager"

#define UDISKS2_MATCH_RULE "type='signal',sender='" UDISKS2_SERVICE_UDISKS2 "',path_namespace='" UDISKS2_PATH_UDISKS2 "'"

CUDisks2Provider::Drive::Drive(const char *object) : m_object(object)
{
}

bool CUDisks2Provider::Drive::IsOptical()
{
  return std::any_of(m_mediaCompatibility.begin(), m_mediaCompatibility.end(), [](std::string kind)
  {
    return kind.compare(0, 7, "optical") == 0;
  });
}

std::string CUDisks2Provider::Drive::toString()
{
  return StringUtils::Format("Drive %s: IsRemovable %s IsOptical %s",
                             m_object, BOOL2SZ(m_isRemovable), BOOL2SZ(IsOptical()));
}

CUDisks2Provider::Block::Block(const char *object) : m_object(object)
{
}

bool CUDisks2Provider::Block::IsReady()
{
  return m_drive != nullptr;
}

std::string CUDisks2Provider::Block::toString()
{
  return StringUtils::Format("Block device %s: Device %s Label %s IsSystem: %s Drive %s",
                             m_object, m_device, m_label.empty() ? "none" : m_label,
                             BOOL2SZ(m_isSystem), m_driveobject.empty() ? "none" : m_driveobject);
}

CUDisks2Provider::Filesystem::Filesystem(const char *object) : m_object(object)
{
}

std::string CUDisks2Provider::Filesystem::toString()
{
  return StringUtils::Format("Filesystem %s: IsMounted %s MountPoint %s",
                             m_object, BOOL2SZ(m_isMounted), m_mountPoint.empty() ? "none" : m_mountPoint);
}

bool CUDisks2Provider::Filesystem::IsReady()
{
  return m_block != nullptr && m_block->IsReady();
}

bool CUDisks2Provider::Filesystem::IsOptical()
{
  return m_block->m_drive->IsOptical();
}

bool CUDisks2Provider::Filesystem::Mount()
{
  if (m_block->m_isSystem) {
    CLog::Log(LOGDEBUG, "UDisks2: Skip mount of system device %s", toString());
    return false;
  }
  else if (m_isMounted)
  {
    CLog::Log(LOGDEBUG, "UDisks2: Skip mount of already mounted device %s", toString());
    return false;
  }
  else
  {
    CLog::Log(LOGDEBUG, "UDisks2: Mounting %s", m_block->m_device);
    CDBusMessage message(UDISKS2_SERVICE_UDISKS2, m_object, UDISKS2_INTERFACE_FILESYSTEM, "Mount");
    AppendEmptyOptions(message.GetArgumentIter());
    DBusMessage *reply = message.SendSystem();
    return (reply && dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_ERROR);
  }
}

bool CUDisks2Provider::Filesystem::Unmount()
{
  if (m_block->m_isSystem) {
    CLog::Log(LOGDEBUG, "UDisks2: Skip unmount of system device %s", toString());
    return false;
  }
  else if (!m_isMounted)
  {
    CLog::Log(LOGDEBUG, "UDisks2: Skip unmount of not mounted device %s", toString());
    return false;
  }
  else
  {
    CLog::Log(LOGDEBUG, "UDisks2: Unmounting %s", m_block->m_device);
    CDBusMessage message(UDISKS2_SERVICE_UDISKS2, m_object, UDISKS2_INTERFACE_FILESYSTEM, "Unmount");
    AppendEmptyOptions(message.GetArgumentIter());
    DBusMessage *reply = message.SendSystem();
    return (reply && dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_ERROR);
  }
}

std::string CUDisks2Provider::Filesystem::GetDisplayName()
{
  if (m_block->m_label.empty())
  {
    std::string strSize = StringUtils::SizeToString(m_block->m_size);
    return StringUtils::Format("%s %s", strSize, g_localizeStrings.Get(155));
  }
  else
    return m_block->m_label;
}

CMediaSource CUDisks2Provider::Filesystem::ToMediaShare()
{
  CMediaSource source;
  source.strPath = m_mountPoint;
  source.strName = GetDisplayName();
  if (IsOptical())
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  else if (m_block->m_isSystem)
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  else
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
  source.m_ignore = true;
  return source;
}

bool CUDisks2Provider::Filesystem::IsApproved()
{
  return IsReady() &&
         (m_isMounted && m_mountPoint != "/" && m_mountPoint != "/boot" && m_mountPoint.compare(0, 6, "/boot/") != 0) /*||
         m_block->m_drive->m_isOptical*/;
}

CUDisks2Provider::CUDisks2Provider()
{
  if (!m_connection.Connect(DBUS_BUS_SYSTEM, true))
  {
    return;
  }

  dbus_connection_set_exit_on_disconnect(m_connection, static_cast<dbus_bool_t>(false));

  CDBusError error;
  dbus_bus_add_match(m_connection, UDISKS2_MATCH_RULE, error);
  dbus_connection_flush(m_connection);

  if (error)
  {
    error.Log("UDisks2: Failed to attach to signal");
    m_connection.Destroy();
  }
}

CUDisks2Provider::~CUDisks2Provider()
{
  for (auto &elt : m_filesystems)
  {
    delete elt.second;
  }
  m_filesystems.clear();

  for (auto &elt : m_blocks)
  {
    delete elt.second;
  }
  m_blocks.clear();

  for (auto &elt : m_drives)
  {
    delete elt.second;
  }
  m_drives.clear();
}

void CUDisks2Provider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected UDisks2 as storage provider");
  m_daemonVersion = CDBusUtil::GetVariant(UDISKS2_SERVICE_UDISKS2, UDISKS2_PATH_MANAGER, UDISKS2_INTERFACE_MANAGER,
                                          "Version").asString();
  CLog::Log(LOGDEBUG, "UDisks2: Daemon version %s", m_daemonVersion);

  CLog::Log(LOGDEBUG, "UDisks2: Querying available devices");
  CDBusMessage message(UDISKS2_SERVICE_UDISKS2, UDISKS2_PATH_UDISKS2, DBUS_INTERFACE_OBJECT_MANAGER,
                       "GetManagedObjects");
  DBusMessage *reply = message.SendSystem();

  if (reply && dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_ERROR)
  {
    HandleManagedObjects(reply);
  }
}

bool CUDisks2Provider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessagePtr msg(dbus_connection_pop_message(m_connection));

    if (msg)
    {
      CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Message received (interface: %s, member: %s)",
                dbus_message_get_interface(msg.get()), dbus_message_get_member(msg.get()));

      if (dbus_message_is_signal(msg.get(), DBUS_INTERFACE_OBJECT_MANAGER, "InterfacesAdded"))
      {
        HandleInterfacesAdded(msg.get());
        return false;
      }
      else if (dbus_message_is_signal(msg.get(), DBUS_INTERFACE_OBJECT_MANAGER, "InterfacesRemoved"))
      {
        return HandleInterfacesRemoved(msg.get(), callback);
      }
      else if (dbus_message_is_signal(msg.get(), DBUS_INTERFACE_PROPERTIES, "PropertiesChanged"))
      {
        return HandlePropertiesChanged(msg.get(), callback);
      }
    }
  }
  return false;
}

bool CUDisks2Provider::HasUDisks2()
{
  return CDBusUtil::TryMethodCall(DBUS_BUS_SYSTEM, UDISKS2_SERVICE_UDISKS2, UDISKS2_PATH_UDISKS2, DBUS_INTERFACE_PEER,
                                  "Ping");
}

bool CUDisks2Provider::Eject(const std::string &mountpath)
{
  std::string path(mountpath);
  URIUtils::RemoveSlashAtEnd(path);

  for (const auto &elt: m_filesystems)
  {
    Filesystem *fs = elt.second;
    if (fs->m_mountPoint == path)
    {
      return fs->Unmount();
    }
  }

  return false;
}

std::vector<std::string> CUDisks2Provider::GetDiskUsage()
{
  CPosixMountProvider legacy;
  return legacy.GetDiskUsage();
}

void CUDisks2Provider::GetDisks(VECSOURCES &devices, bool enumerateRemovable)
{
  for (const auto &elt: m_filesystems)
  {
    Filesystem *fs = elt.second;
    if (fs->IsApproved() && fs->m_block->m_isSystem != enumerateRemovable)
      devices.push_back(fs->ToMediaShare());
  }
}

void CUDisks2Provider::DriveAdded(Drive *drive)
{
  CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Drive added - %s", drive->toString());

  if (m_drives[drive->m_object])
  {
    CLog::Log(LOGWARNING, "UDisks2: Inconsistency found! DriveAdded on an indexed drive");
    delete m_drives[drive->m_object];
  }

  m_drives[drive->m_object] = drive;

  for (auto &elt: m_blocks)
  {
    auto block = elt.second;
    if (block->m_driveobject == drive->m_object)
    {
      block->m_drive = drive;
      BlockAdded(block, false);
    }
  }
}

bool CUDisks2Provider::DriveRemoved(std::string object)
{
  CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Drive removed (%s)", object);

  if (m_drives.count(object) > 0)
  {
    delete m_drives[object];
    m_drives.erase(object);
  }

  for (auto &elt: m_blocks)
  {
    auto block = elt.second;
    if (block->m_driveobject == object)
    {
      block->m_drive = nullptr;
    }
  }

  return false;
}

void CUDisks2Provider::BlockAdded(Block *block, bool isNew)
{
  if (isNew)
  {
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Block added - %s", block->toString());

    if (m_drives.count(block->m_driveobject) > 0)
      block->m_drive = m_drives[block->m_driveobject];

    if (m_blocks[block->m_object])
    {
      CLog::Log(LOGWARNING, "UDisks2: Inconsistency found! BlockAdded on an indexed block device");
      delete m_blocks[block->m_object];
    }

    m_blocks[block->m_object] = block;
  }


  if (m_filesystems.count(block->m_object) > 0)
  {
    auto fs = m_filesystems[block->m_object];
    fs->m_block = block;
    FilesystemAdded(fs, false);
  }
}

bool CUDisks2Provider::BlockRemoved(std::string object)
{
  CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Block removed (%s)", object);

  if (m_blocks.count(object) > 0)
  {
    delete m_blocks[object];
    m_blocks.erase(object);
  }

  if (m_filesystems.count(object) > 0)
  {
    m_filesystems[object]->m_block = nullptr;
  }

  return false;
}

void CUDisks2Provider::FilesystemAdded(Filesystem *fs, bool isNew)
{
  if (isNew)
  {
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Filesystem added - %s", fs->toString());

    if (m_blocks.count(fs->m_object) > 0)
      fs->m_block = m_blocks[fs->m_object];

    if (m_filesystems[fs->m_object])
    {
      CLog::Log(LOGWARNING, "UDisks2: Inconsistency found! FilesystemAdded on an indexed filesystem");
      delete m_filesystems[fs->m_object];
    }

    m_filesystems[fs->m_object] = fs;
  }

  if (fs->IsReady() && !fs->m_isMounted && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_handleMounting)
  {
    fs->Mount();
  }
}

bool CUDisks2Provider::FilesystemRemoved(const char *object, IStorageEventsCallback *callback)
{
  CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Filesystem removed (%s)", object);
  bool result = false;
  if (m_filesystems.count(object) > 0)
  {
    auto fs = m_filesystems[object];
    if (fs->m_isMounted)
    {
      callback->OnStorageUnsafelyRemoved(fs->GetDisplayName());
      result = true;
    }
    delete m_filesystems[object];
    m_filesystems.erase(object);
  }
  return result;
}

void CUDisks2Provider::HandleManagedObjects(DBusMessage *msg)
{
  DBusMessageIter msgIter, dictIter;
  dbus_message_iter_init(msg, &msgIter);
  dbus_message_iter_recurse(&msgIter, &dictIter);
  while (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY)
  {
    DBusMessageIter objIter;
    dbus_message_iter_recurse(&dictIter, &objIter);
    ParseInterfaces(&objIter);
    dbus_message_iter_next(&dictIter);
  }
}

void CUDisks2Provider::HandleInterfacesAdded(DBusMessage *msg)
{
  DBusMessageIter msgIter;
  dbus_message_iter_init(msg, &msgIter);
  ParseInterfaces(&msgIter);
}

bool CUDisks2Provider::HandleInterfacesRemoved(DBusMessage *msg, IStorageEventsCallback *callback)
{
  DBusMessageIter msgIter, ifaceIter;
  const char *path, *iface;
  bool result = false;
  dbus_message_iter_init(msg, &msgIter);
  dbus_message_iter_get_basic(&msgIter, &path);
  dbus_message_iter_next(&msgIter);
  dbus_message_iter_recurse(&msgIter, &ifaceIter);
  while (dbus_message_iter_get_arg_type(&ifaceIter) == DBUS_TYPE_STRING)
  {
    dbus_message_iter_get_basic(&ifaceIter, &iface);
    result |= RemoveInterface(path, iface, callback);
    dbus_message_iter_next(&ifaceIter);
  }
  return result;
}

bool CUDisks2Provider::HandlePropertiesChanged(DBusMessage *msg, IStorageEventsCallback *callback)
{
  DBusMessageIter msgIter, propsIter;
  const char *object = dbus_message_get_path(msg);
  const char *iface;

  dbus_message_iter_init(msg, &msgIter);
  dbus_message_iter_get_basic(&msgIter, &iface);
  dbus_message_iter_next(&msgIter);
  dbus_message_iter_recurse(&msgIter, &propsIter);

  if (strcmp(iface, UDISKS2_INTERFACE_DRIVE) == 0)
  {
    return DrivePropertiesChanged(object, &propsIter);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_BLOCK) == 0)
  {
    return BlockPropertiesChanged(object, &propsIter);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_FILESYSTEM) == 0)
  {
    return FilesystemPropertiesChanged(object, &propsIter, callback);
  }

  return false;
}

bool CUDisks2Provider::DrivePropertiesChanged(const char *object, DBusMessageIter *propsIter)
{
  if (m_drives.count(object) > 0)
  {
    auto drive = m_drives[object];
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Before update: %s", drive->toString());
    auto ParseDriveProperty = std::bind(&CUDisks2Provider::ParseDriveProperty, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3);
    ParseProperties(drive, propsIter, ParseDriveProperty);
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: After update: %s", drive->toString());
  }
  return false;
}

bool CUDisks2Provider::BlockPropertiesChanged(const char *object, DBusMessageIter *propsIter)
{
  if (m_blocks.count(object) > 0)
  {
    auto block = m_blocks[object];
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Before update: %s", block->toString());
    auto ParseBlockProperty = std::bind(&CUDisks2Provider::ParseBlockProperty, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3);
    ParseProperties(block, propsIter, ParseBlockProperty);
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: After update: %s", block->toString());
  }
  return false;
}

bool CUDisks2Provider::FilesystemPropertiesChanged(const char *object, DBusMessageIter *propsIter, IStorageEventsCallback *callback)
{
  if (m_filesystems.count(object) > 0)
  {
    auto fs = m_filesystems[object];
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: Before update: %s", fs->toString());
    bool wasMounted = fs->m_isMounted;
    auto ParseFilesystemProperty = std::bind(&CUDisks2Provider::ParseFilesystemProperty, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2, std::placeholders::_3);
    ParseProperties(fs, propsIter, ParseFilesystemProperty);
    CLog::Log(LOGDEBUG, LOGDBUS, "UDisks2: After update: %s", fs->toString());

    if (!wasMounted && fs->m_isMounted && fs->IsApproved())
    {
      CLog::Log(LOGINFO, "UDisks2: Added %s", fs->m_mountPoint);
      if (callback)
        callback->OnStorageAdded(fs->GetDisplayName(), fs->m_mountPoint);
      return true;
    }
    else if (wasMounted && !fs->m_isMounted)
    {
      CLog::Log(LOGINFO, "UDisks2: Removed %s", fs->m_block->m_device);
      if (callback)
        callback->OnStorageSafelyRemoved(fs->GetDisplayName());
      return true;
    }
  }
  return false;
}

bool CUDisks2Provider::RemoveInterface(const char *path, const char *iface, IStorageEventsCallback *callback)
{
  if (strcmp(iface, UDISKS2_INTERFACE_DRIVE) == 0)
  {
    return DriveRemoved(path);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_BLOCK) == 0)
  {
    return BlockRemoved(path);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_FILESYSTEM) == 0)
  {
    return FilesystemRemoved(path, callback);
  }
  else
  {
    return false;
  }
}


void CUDisks2Provider::ParseInterfaces(DBusMessageIter *objIter)
{
  DBusMessageIter dictIter;
  const char *object;
  dbus_message_iter_get_basic(objIter, &object);
  dbus_message_iter_next(objIter);
  dbus_message_iter_recurse(objIter, &dictIter);
  while (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY)
  {
    DBusMessageIter ifaceIter, propsIter;
    const char *iface;
    dbus_message_iter_recurse(&dictIter, &ifaceIter);
    dbus_message_iter_get_basic(&ifaceIter, &iface);
    dbus_message_iter_next(&ifaceIter);
    dbus_message_iter_recurse(&ifaceIter, &propsIter);
    ParseInterface(object, iface, &propsIter/*, &discovery*/);
    dbus_message_iter_next(&dictIter);
  }
}

void CUDisks2Provider::ParseInterface(const char *object, const char *iface, DBusMessageIter *propsIter)
{
  if (strcmp(iface, UDISKS2_INTERFACE_DRIVE) == 0)
  {
    auto *drive = new Drive(object);
    auto f = std::bind(&CUDisks2Provider::ParseDriveProperty, this, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3);
    ParseProperties(drive, propsIter, f);
    DriveAdded(drive);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_BLOCK) == 0)
  {
    auto *block = new Block(object);
    auto f = std::bind(&CUDisks2Provider::ParseBlockProperty, this, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3);
    ParseProperties(block, propsIter, f);
    BlockAdded(block);
  }
  else if (strcmp(iface, UDISKS2_INTERFACE_FILESYSTEM) == 0)
  {
    auto *fs = new Filesystem(object);
    auto f = std::bind(&CUDisks2Provider::ParseFilesystemProperty, this, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3);
    ParseProperties(fs, propsIter, f);
    FilesystemAdded(fs);
  }
}


template<class Object, class Function>
void CUDisks2Provider::ParseProperties(Object *ref, DBusMessageIter *propsIter, Function f)
{
  while (dbus_message_iter_get_arg_type(propsIter) == DBUS_TYPE_DICT_ENTRY)
  {
    DBusMessageIter propIter, varIter;
    const char *key;

    dbus_message_iter_recurse(propsIter, &propIter);

    dbus_message_iter_get_basic(&propIter, &key);
    dbus_message_iter_next(&propIter);

    dbus_message_iter_recurse(&propIter, &varIter);

    f(ref, key, &varIter);

    dbus_message_iter_next(propsIter);
  }

}

void CUDisks2Provider::ParseDriveProperty(Drive *drive, const char *key, DBusMessageIter *varIter)
{
  switch (dbus_message_iter_get_arg_type(varIter))
  {
    case DBUS_TYPE_BOOLEAN:
    {
      dbus_bool_t value;

      if (strcmp(key, "Removable") == 0)
      {
        dbus_message_iter_get_basic(varIter, &value);
        drive->m_isRemovable = static_cast<bool>(value);
      }

      break;
    }
    case DBUS_TYPE_ARRAY:
    {
      DBusMessageIter arrIter;

      if (strcmp(key, "MediaCompatibility") == 0)
      {
        dbus_message_iter_recurse(varIter, &arrIter);
        while (dbus_message_iter_get_arg_type(&arrIter) == DBUS_TYPE_STRING)
        {
          const char *compatibility;
          dbus_message_iter_get_basic(&arrIter, &compatibility);
          drive->m_mediaCompatibility.push_back(std::string(compatibility));
          dbus_message_iter_next(&arrIter);
        }
      }

      break;
    }
    default:
      break;
  }
}


void CUDisks2Provider::ParseBlockProperty(Block *block, const char *key, DBusMessageIter *varIter)
{
  switch (dbus_message_iter_get_arg_type(varIter))
  {
    case DBUS_TYPE_OBJECT_PATH:
    {
      const char *value;

      if (strcmp(key, "Drive") == 0)
      {
        dbus_message_iter_get_basic(varIter, &value);
        block->m_driveobject.assign(value);
      }

      break;
    }
    case DBUS_TYPE_STRING:
    {
      const char *value;

      if (strcmp(key, "IdLabel") == 0)
      {
        dbus_message_iter_get_basic(varIter, &value);
        block->m_label.assign(value);
      }

      break;
    }
    case DBUS_TYPE_BOOLEAN:
    {
      dbus_bool_t value;

      if (strcmp(key, "HintSystem") == 0)
      {
        dbus_message_iter_get_basic(varIter, &value);
        block->m_isSystem = static_cast<bool>(value);
      }

      break;
    }
    case DBUS_TYPE_UINT64:
    {
      dbus_uint64_t value;

      if (strcmp(key, "Size") == 0)
      {
        dbus_message_iter_get_basic(varIter, &value);
        block->m_size = value;
      }

      break;
    }
    case DBUS_TYPE_ARRAY:
    {
      DBusMessageIter arrIter;

      if (strcmp(key, "PreferredDevice") == 0)
      {
        dbus_message_iter_recurse(varIter, &arrIter);
        block->m_device.assign(ParseByteArray(&arrIter));
      }

      break;
    }
    default:
      break;
  }
}

void CUDisks2Provider::ParseFilesystemProperty(Filesystem *fs, const char *key, DBusMessageIter *varIter)
{
  switch (dbus_message_iter_get_arg_type(varIter))
  {
    case DBUS_TYPE_ARRAY:
    {
      DBusMessageIter arrIter;

      if (strcmp(key, "MountPoints") == 0)
      {
        dbus_message_iter_recurse(varIter, &arrIter);

        if (dbus_message_iter_get_arg_type(&arrIter) == DBUS_TYPE_ARRAY)
        {
          DBusMessageIter valIter;

          dbus_message_iter_recurse(&arrIter, &valIter);
          fs->m_mountPoint.assign(ParseByteArray(&valIter));

          dbus_message_iter_next(&arrIter);
        }
        else
        {
          fs->m_mountPoint.clear();
        }
        fs->m_isMounted = !fs->m_mountPoint.empty();
      }

      break;
    }
    default:
      break;
  }
}

std::string CUDisks2Provider::ParseByteArray(DBusMessageIter *arrIter)
{
  std::ostringstream strStream;

  while (dbus_message_iter_get_arg_type(arrIter) == DBUS_TYPE_BYTE)
  {
    dbus_int16_t value = 0;
    dbus_message_iter_get_basic(arrIter, &value);
    if (value == 0)
      break;
    strStream << static_cast<char>(value);
    dbus_message_iter_next(arrIter);
  }

  return strStream.str();
}

void CUDisks2Provider::AppendEmptyOptions(DBusMessageIter *argsIter)
{
  DBusMessageIter dictIter;
  dbus_message_iter_open_container(argsIter, DBUS_TYPE_ARRAY, "{sv}", &dictIter);
  dbus_message_iter_close_container(argsIter, &dictIter);
}

