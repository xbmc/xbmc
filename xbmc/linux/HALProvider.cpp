#include "HALProvider.h"
#ifdef HAS_HAL
#include "HALManager.h"

CHALProvider::CHALProvider()
{
  m_removableLength = 0;
  PumpDriveChangeEvents();
}

void CHALProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  std::vector<CStorageDevice> devices = g_HalManager.GetVolumeDevices();

  for (size_t i = 0; i < devices.size(); i++)
  {
    if (devices[i].Mounted && devices[i].Approved && !devices[i].HotPlugged)
    {
      CMediaSource share;
      devices[i].toMediaSource(&share);
      localDrives.push_back(share);
    }
  }
}

void CHALProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  std::vector<CStorageDevice> devices = g_HalManager.GetVolumeDevices();

  for (size_t i = 0; i < devices.size(); i++)
  {
    if (devices[i].Mounted && devices[i].Approved && devices[i].HotPlugged)
    {
      CMediaSource share;
      devices[i].toMediaSource(&share);
      removableDrives.push_back(share);
    }
  }
}

std::vector<CStdString> CHALProvider::GetDiskUsage()
{
  return std::vector<CStdString>();
}

// TODO Use HALs events for this instead.
bool CHALProvider::PumpDriveChangeEvents()
{
//Pump HalManager dry of events
  while(g_HalManager.Update()) ;

  VECSOURCES drives;
  GetRemovableDrives(drives);
  bool changed = drives.size() != m_removableLength;
  m_removableLength = drives.size();
  return changed;
}
#endif
