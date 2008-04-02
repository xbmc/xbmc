#pragma once

#include <vector>
#include "../Settings.h"  // for VECSOURCES

class IDevice;
class IFileSystem;

class CMemoryUnitManager
{
public:
  CMemoryUnitManager();

  // update the memory units (plug, unplug)
  bool Update();

  bool IsDriveValid(char Drive);    // for backward compatibility
                                    // with fatx drives in filezilla

  IDevice *GetDevice(unsigned char unit) const;
  IFileSystem *GetFileSystem(unsigned char unit);

  bool IsDriveWriteable(const CStdString &path) const;

  void GetMemoryUnitSources(VECSOURCES &shares);


private:
  void Notify(unsigned long port, unsigned long slot, bool success);

  bool HasDevice(unsigned long port, unsigned long slot);
  bool MountDevice(unsigned long port, unsigned long slot);
  bool UnMountDevice(unsigned long port, unsigned long slot);
  
  void MountUnits(unsigned long device, bool notify);
  void UnMountUnits(unsigned long device);

  char DriveLetterFromPort(unsigned long port, unsigned long slot);

  void DumpImage(const CStdString &path, unsigned char unit, unsigned long sectors);

  std::vector<IDevice *> m_memUnits;

  bool m_initialized;
};

extern CMemoryUnitManager g_memoryUnitManager;
