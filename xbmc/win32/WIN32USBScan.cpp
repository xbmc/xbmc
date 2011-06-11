#include <setupapi.h>
#include "WIN32USBScan.h"

static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

CWIN32USBScan::CWIN32USBScan()
{
  HDEVINFO hDevInfo;
  SP_DEVINFO_DATA DeviceInfoData;

  hDevInfo = SetupDiGetClassDevs(&USB_HID_GUID, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
  {

  }
}