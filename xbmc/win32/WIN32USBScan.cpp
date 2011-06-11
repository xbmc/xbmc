#include <setupapi.h>
#include "WIN32USBScan.h"

CWIN32USBScan::CWIN32USBScan()
{
  HDEVINFO hDevInfo;
  SP_DEVINFO_DATA DeviceInfoData;

  hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
  DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
  {

  }
}