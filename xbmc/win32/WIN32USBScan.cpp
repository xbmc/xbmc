#include <setupapi.h>
#include "WIN32USBScan.h"
#include "input/KeymapLoader.h"

static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

CWIN32USBScan::CWIN32USBScan()
{
  HDEVINFO hDevHandle;
  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  DWORD required = 0;
  deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  int nBufferSize = 0;
      
  SP_DEVINFO_DATA devInfoData;
  devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

  DWORD MemberIndex = 0;
  BOOL  Result;

  hDevHandle = SetupDiGetClassDevs(&USB_HID_GUID, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

  if (hDevHandle == INVALID_HANDLE_VALUE)
    return;

  bool bStart = false;
  TCHAR *buffer = NULL;
  PSP_DEVICE_INTERFACE_DETAIL_DATA devicedetailData;
	do
	{
		Result = SetupDiEnumDeviceInfo(hDevHandle, MemberIndex, &devInfoData);

		if (Result)
			Result = SetupDiEnumDeviceInterfaces(hDevHandle, 0, &USB_HID_GUID, MemberIndex, &deviceInterfaceData);
           
    if(!Result)
    {
      SetupDiDestroyDeviceInfoList(hDevHandle);
      delete []buffer;
      buffer = NULL;
      return;
    }

    MemberIndex++;
    BOOL detailResult = false;
      
    if(!bStart)
    {
      // As per MSDN, Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with a 
      // NULL DeviceInterfaceDetailData pointer, a DeviceInterfaceDetailDataSize of zero, 
      // and a valid RequiredSize variable. In response to such a call, this function returns 
      // the required buffer size at RequiredSize and fails with GetLastError returning 
      // ERROR_INSUFFICIENT_BUFFER. 
      // Allocate an appropriately sized buffer and call the function again to get the interface details. 

      SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, NULL, 0, &required, NULL);

      buffer = new TCHAR[required];
      devicedetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) buffer;
      devicedetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
      nBufferSize = required;
      bStart = true;
    }

    detailResult = SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, devicedetailData, nBufferSize , &required, NULL);

    CKeymapLoader::DeviceAdded(CKeymapLoader::ParseWin32HIDName(devicedetailData->DevicePath));

    if(!detailResult)
        continue;

  } while(Result);
}