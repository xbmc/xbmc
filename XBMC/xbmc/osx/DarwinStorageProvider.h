#pragma once
#include "IStorageProvider.h"

class CDarwinStorageProvider : public IStorageProvider
{
public:
  CDarwinStorageProvider();
  virtual ~CDarwinStorageProvider() { }

  virtual void Initialize() { }
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDrives(localDrives); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { /*GetDrives(removableDrives);*/ }

  virtual std::vector<CStdString> GetDiskUsage(void);

  virtual bool Eject(CStdString mountpath);

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

  static void SetEvent(void);

private:
  void GetDrives(VECSOURCES &drives);

  unsigned int m_removableLength;
  static bool m_event;
};
