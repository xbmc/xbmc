#pragma once

#include "IDevice.h"

class CFatXDevice : public IDevice
{
public:
  CFatXDevice(unsigned long port, unsigned long slot, void *device);

  virtual void LogInfo();
  virtual const char *GetFileSystem();
  virtual void UnMount();
  virtual bool Mount(const char *device);

  char GetDrive() { return m_drive; };

  bool ReadVolumeName();
protected:
  char m_drive;
};
