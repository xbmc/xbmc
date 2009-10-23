#pragma once
#include "system.h"
#include "MediaSource.h"

class IStorageEventsCallback
{
public:
  virtual ~IStorageEventsCallback() { }

  virtual void OnStorageAdded(const CStdString &label, const CStdString &path) = 0;
  virtual void OnStorageSafelyRemoved(const CStdString &label) = 0;
  virtual void OnStorageUnsafelyRemoved(const CStdString &label) = 0;
};

class IStorageProvider
{
public:
  virtual ~IStorageProvider() { }

  virtual void Initialize() = 0;
  virtual void Stop() = 0;

  virtual void GetLocalDrives(VECSOURCES &localDrives) = 0;
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) = 0;

  virtual bool Eject(CStdString mountpath) = 0;

  virtual std::vector<CStdString> GetDiskUsage() = 0;

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback) = 0;
};
