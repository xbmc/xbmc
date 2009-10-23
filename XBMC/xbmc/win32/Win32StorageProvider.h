#pragma once
#include "IStorageProvider.h"

class CWin32StorageProvider : public IStorageProvider
{
public:
  virtual ~CWin32StorageProvider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents() { return false; }
};
