/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <vector>

#include "storage/IStorageProvider.h"
#include "DBusUtil.h"

class CUDisks2Provider : public IStorageProvider
{
  class Drive
  {
  public:
    std::string m_object;
    bool m_isRemovable = false;
    std::vector<std::string> m_mediaCompatibility;

    explicit Drive(const char *object);
    ~Drive() = default;

    bool IsOptical();

    std::string toString();
  };

  class Block
  {
  public:
    Drive *m_drive = nullptr;
    std::string m_object;
    std::string m_driveobject;
    std::string m_label;
    std::string m_device;
    bool m_isSystem = false;
    u_int64_t m_size = 0;

    explicit Block(const char *object);
    ~Block() = default;

    bool IsReady();

    std::string toString();
  };

  class Filesystem
  {
  public:
    Block *m_block = nullptr;
    std::string m_object;
    std::string m_mountPoint;
    bool m_isMounted = false;

    explicit Filesystem(const char *object);
    ~Filesystem() = default;

    bool IsReady();
    bool IsOptical();

    bool Mount();
    bool Unmount();

    std::string GetDisplayName();
    CMediaSource ToMediaShare();
    bool IsApproved();

    std::string toString();
  };

  typedef std::map<std::string, Drive *> DriveMap;
  typedef std::map<std::string, Block *> BlockMap;
  typedef std::map<std::string, Filesystem *> FilesystemMap;

public:
  CUDisks2Provider();
  ~CUDisks2Provider() override;

  void Initialize() override;

  bool PumpDriveChangeEvents(IStorageEventsCallback *callback) override;

  static bool HasUDisks2();

  bool Eject(const std::string &mountpath) override;

  std::vector<std::string> GetDiskUsage() override;

  void GetLocalDrives(VECSOURCES &localDrives) override
  { GetDisks(localDrives, false); }

  void GetRemovableDrives(VECSOURCES &removableDrives) override
  { GetDisks(removableDrives, true); }

  void Stop() override
  {}

private:
  CDBusConnection m_connection;

  DriveMap m_drives;
  BlockMap m_blocks;
  FilesystemMap m_filesystems;

  std::string m_daemonVersion;

  void GetDisks(VECSOURCES &devices, bool enumerateRemovable);

  void DriveAdded(Drive *drive);
  bool DriveRemoved(std::string object);
  void BlockAdded(Block *block, bool isNew = true);
  bool BlockRemoved(std::string object);
  void FilesystemAdded(Filesystem *fs, bool isNew = true);
  bool FilesystemRemoved(const char *object, IStorageEventsCallback *callback);

  bool HandleInterfacesRemoved(DBusMessage *msg, IStorageEventsCallback *callback);
  void HandleInterfacesAdded(DBusMessage *msg);
  bool HandlePropertiesChanged(DBusMessage *msg, IStorageEventsCallback *callback);

  bool DrivePropertiesChanged(const char *object, DBusMessageIter *propsIter);
  bool BlockPropertiesChanged(const char *object, DBusMessageIter *propsIter);
  bool FilesystemPropertiesChanged(const char *object, DBusMessageIter *propsIter, IStorageEventsCallback *callback);

  bool RemoveInterface(const char *path, const char *iface, IStorageEventsCallback *callback);

  template<class Object, class Function>
  void ParseProperties(Object *ref, DBusMessageIter *dictIter, Function f);
  void ParseInterfaces(DBusMessageIter *dictIter);
  void ParseDriveProperty(Drive *drive, const char *key, DBusMessageIter *varIter);
  void ParseBlockProperty(Block *block, const char *key, DBusMessageIter *varIter);
  void ParseFilesystemProperty(Filesystem *fs, const char *key, DBusMessageIter *varIter);
  std::string ParseByteArray(DBusMessageIter *arrIter);
  void HandleManagedObjects(DBusMessage *msg);
  void ParseInterface(const char *object, const char *iface, DBusMessageIter *propsIter);

  static void AppendEmptyOptions(DBusMessageIter *argsIter);
};
