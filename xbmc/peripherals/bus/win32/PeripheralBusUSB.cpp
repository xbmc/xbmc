/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralBusUSB.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

static GUID USB_RAW_GUID =  { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
static GUID USB_HID_GUID =  { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
static GUID USB_DISK_GUID = { 0x53F56307, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };
static GUID USB_NIC_GUID =  { 0xAD498944, 0x762F, 0x11D0, { 0x8D, 0xCB, 0x00, 0xC0, 0x4F, 0xC3, 0x35, 0x8C } };

using namespace PERIPHERALS;

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals *manager) :
    CPeripheralBus(manager, PERIPHERAL_BUS_USB)
{
  /* device removals aren't always triggering OnDeviceRemoved events, so poll for changes every 5 seconds to be sure we don't miss anything */
  m_iRescanTime = 5000;
}

bool CPeripheralBusUSB::PerformDeviceScan(PeripheralScanResults &results)
{
  /* XXX we'll just scan the RAW guid and find all devices. they'll show up as type 'unknown' in the UI,
     but the other option is that they're detected more than once, because RAW will return all devices.
     we have to scan the RAW guid here, because not every device is found by it's GUID correctly, e.g. CDC adapters. */
  return PerformDeviceScan(&USB_RAW_GUID, PERIPHERAL_UNKNOWN, results);
}

bool CPeripheralBusUSB::PerformDeviceScan(const GUID *guid, const PeripheralType type, PeripheralScanResults &results)
{
  bool     bReturn(false);
  HDEVINFO hDevHandle;
  DWORD    required = 0, iMemberIndex = 0;
  int      nBufferSize = 0;

  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  SP_DEVINFO_DATA devInfoData;
  devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

  if ((hDevHandle = SetupDiGetClassDevs(guid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGWARNING, "%s - cannot query USB devices: invalid handle", __FUNCTION__);
    return bReturn;
  }

  bReturn = true;
  BOOL bResult = true;
  PSP_DEVICE_INTERFACE_DETAIL_DATA devicedetailData = NULL;
  while(bResult)
  {
    bResult = SetupDiEnumDeviceInfo(hDevHandle, iMemberIndex, &devInfoData);

    if (bResult)
      bResult = SetupDiEnumDeviceInterfaces(hDevHandle, 0, guid, iMemberIndex, &deviceInterfaceData);

    if (bResult)
    {
      iMemberIndex++;
      BOOL bDetailResult = false;
      {
        // As per MSDN, Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with a 
        // NULL DeviceInterfaceDetailData pointer, a DeviceInterfaceDetailDataSize of zero, 
        // and a valid RequiredSize variable. In response to such a call, this function returns 
        // the required buffer size at RequiredSize and fails with GetLastError returning 
        // ERROR_INSUFFICIENT_BUFFER. 
        // Allocate an appropriately sized buffer and call the function again to get the interface details. 

        SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, NULL, 0, &required, NULL);

        devicedetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(required * sizeof(TCHAR));
        devicedetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        nBufferSize = required;
      }

      bDetailResult = SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, devicedetailData, nBufferSize , &required, NULL);
      if (bDetailResult)
      {
        CStdString strVendorId(StringUtils::EmptyString);
        CStdString strProductId(StringUtils::EmptyString);
        CStdString strTmp(devicedetailData->DevicePath);
        strVendorId = strTmp.substr(strTmp.Find("vid_") + 4, 4);
        strProductId = strTmp.substr(strTmp.Find("pid_") + 4, 4);

        if ((strTmp.Find("&mi_") < 0) || (strTmp.Find("&mi_00") >= 0))
        {
          PeripheralScanResult prevDevice;
          if (!results.GetDeviceOnLocation(devicedetailData->DevicePath, &prevDevice))
          {
            PeripheralScanResult result;
            result.m_strLocation  = devicedetailData->DevicePath;
            result.m_type         = type;
            result.m_iVendorId    = PeripheralTypeTranslator::HexStringToInt(strVendorId.c_str());
            result.m_iProductId   = PeripheralTypeTranslator::HexStringToInt(strProductId.c_str());

            if (!results.ContainsResult(result))
              results.m_results.push_back(result);
          }
        }
      }

      if (devicedetailData)
      {
        free(devicedetailData);
        devicedetailData = NULL;
      } 
    }
    else
    {
      SetupDiDestroyDeviceInfoList(hDevHandle);
      return bReturn;
    }
  }

  return bReturn;
}
