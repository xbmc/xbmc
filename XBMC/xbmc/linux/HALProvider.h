#pragma once
#include "IStorageProvider.h"
#ifdef HAS_HAL

class CHALProvider : public IStorageProvider
{
public:
  CHALProvider();
  virtual ~CHALProvider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();
private:
  unsigned int m_removableLength;
};
#endif
