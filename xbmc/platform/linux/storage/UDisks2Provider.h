/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DBusUtil.h"
#include "storage/IStorageProvider.h"

#include <string>
#include <vector>

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

    /*! \brief Check if the drive is optical
      * @return true if the drive is optical, false otherwise
    */
    bool IsOptical() const;

    /*! \brief Get a representation of the drive as a readable string
      * @return drive as a string
    */
    std::string ToString() const;
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

    /*! \brief Get a representation of the block as a readable string
      * @return block as a string
    */
    std::string ToString() const;
  };

  class Filesystem
  {
  public:
    Block *m_block = nullptr;

    explicit Filesystem(const char *object);
    ~Filesystem() = default;

    bool Mount();
    bool Unmount();

    /*! \brief Get the device display name/label
     * @return the device display name/label
    */
    std::string GetDisplayName() const;

    /*! \brief Check if the device is approved/whitelisted
     * @return true if the device is approved/whitelisted, false otherwise
    */
    bool IsApproved() const;

    /*! \brief Check if the device is mounted
     * @return true if the device is mounted, false otherwise
    */
    bool IsMounted() const;

    /*! \brief Check if the device is ready
     * @return true if the device is ready, false otherwise
    */
    bool IsReady() const;

    /*! \brief Check if the device is optical
     * @return true if the device is optical, false otherwise
    */
    bool IsOptical() const;

    /*! \brief Get the storage type of this device
     * @return the storage type (e.g. OPTICAL) or UNKNOWN if
     * the type couldn't be detected
    */
    MEDIA_DETECT::STORAGE::Type GetStorageType() const;

    /*! \brief Get the device mount point
     * @return the device mount point
    */
    const std::string& GetMountPoint() const { return m_mountPoint; }

    /*! \brief Reset the device mount point
    */
    void ResetMountPoint();

    /*! \brief Set the device mount point
     * @param mountPoint the device mount point
    */
    void SetMountPoint(const std::string& mountPoint);

    /*! \brief Get the device dbus object
     * @return the device dbus object
    */
    const std::string& GetObject() const { return m_object; }

    /*! \brief Get a representation of the device as a readable string
     * @return device as a string
    */
    std::string ToString() const;

    /*! \brief Get a representation of the device as a media share
     * @return the media share
    */
    CMediaSource ToMediaShare() const;

    /*! \brief Get a representation of the device as a storage device abstraction
     * @return the storage device abstraction of the device
    */
    MEDIA_DETECT::STORAGE::StorageDevice ToStorageDevice() const;

  private:
    bool m_isMounted = false;
    std::string m_object;
    std::string m_mountPoint;
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

  void GetLocalDrives(std::vector<CMediaSource>& localDrives) override
  { GetDisks(localDrives, false); }

  void GetRemovableDrives(std::vector<CMediaSource>& removableDrives) override
  { GetDisks(removableDrives, true); }

  void Stop() override
  {}

private:
  CDBusConnection m_connection;

  DriveMap m_drives;
  BlockMap m_blocks;
  FilesystemMap m_filesystems;

  std::string m_daemonVersion;

  void GetDisks(std::vector<CMediaSource>& devices, bool enumerateRemovable);

  void DriveAdded(Drive *drive);
  bool DriveRemoved(const std::string& object);
  void BlockAdded(Block *block, bool isNew = true);
  bool BlockRemoved(const std::string& object);
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
