#pragma once
#include "IStorageProvider.h"

class CPosixMountProvider : public IStorageProvider
{
public:
  CPosixMountProvider();
  virtual ~CPosixMountProvider() { }

  virtual void Initialize() { }
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDrives(localDrives); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { /*GetDrives(removableDrives);*/ }

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool Eject(CStdString mountpath) { return false; }

  virtual bool PumpDriveChangeEvents();
private:
  void GetDrives(VECSOURCES &drives);

  unsigned int m_removableLength;
};
