#pragma once
#include "IStorageProvider.h"

class CPosixMountProvider : public IStorageProvider
{
public:
  CPosixMountProvider();
  virtual ~CPosixMountProvider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDrives(localDrives); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { /*GetDrives(removableDrives);*/ }

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();
private:
  void GetDrives(VECSOURCES &drives);

  unsigned int m_removableLength;
};
