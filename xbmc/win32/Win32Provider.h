#pragma once
#include "IStorageProvider.h"

class CWin32Provider : public IStorageProvider
{
public:
  virtual ~CWin32Provider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents() { return false; }
};
