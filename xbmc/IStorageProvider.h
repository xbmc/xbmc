#pragma once
#include "system.h"
#include "MediaSource.h"

class IStorageProvider
{
public:
  virtual ~IStorageProvider() { }

  virtual void Initialize() = 0;

  virtual void GetLocalDrives(VECSOURCES &localDrives) = 0;
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) = 0;

  virtual std::vector<CStdString> GetDiskUsage() = 0;

  virtual bool PumpDriveChangeEvents() = 0;
};
