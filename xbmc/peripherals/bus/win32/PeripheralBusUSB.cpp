/*
 *      Copyright (C) 2005-2013 Team XBMC
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

const static GUID USB_RAW_GUID = { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
const static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
const static GUID USB_DISK_GUID = { 0x53F56307, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };
const static GUID USB_NIC_GUID = { 0xAD498944, 0x762F, 0x11D0, { 0x8D, 0xCB, 0x00, 0xC0, 0x4F, 0xC3, 0x35, 0x8C } };

using namespace PERIPHERALS;

// Only to avoid endless loops while scanning for devices
#define MAX_BUS_DEVICES 2000

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals& manager) :
    CPeripheralBus("PeripBusUSB", manager, PERIPHERAL_BUS_USB)
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

bool CPeripheralBusUSB::PerformDeviceScan(const GUID *guid, const PeripheralType defaultType, PeripheralScanResults &results)
{
  bool     bReturn(false);
  DWORD    required = 0, iMemberIndex = 0;
  int      nBufferSize = 200;  // Just initial guess, will be increased if required

  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  SP_DEVINFO_DATA devInfoData;
  devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

  HDEVINFO const hDevHandle = SetupDiGetClassDevs(guid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (hDevHandle == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGWARNING, "%s - cannot query USB devices: invalid handle", __FUNCTION__);
    return bReturn;
  }

  PSP_DEVICE_INTERFACE_DETAIL_DATA devicedetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(nBufferSize);
  int nPropertyBufferSize = 100; // Just initial guess, will be increased if required
  char* deviceProperty = (char*)malloc(nPropertyBufferSize);
  if (!devicedetailData || !deviceProperty)
  {
    free(devicedetailData);
    free(deviceProperty);
    CLog::Log(LOGSEVERE, "%s: memory allocation failed", __FUNCTION__);
    return false;
  }

  SP_DEVINFO_DATA deviceInfo;
  bReturn = true;
  for (iMemberIndex = 0; bReturn && iMemberIndex < MAX_BUS_DEVICES; iMemberIndex++)
  {
    bReturn = SetupDiEnumDeviceInfo(hDevHandle, iMemberIndex, &devInfoData) == TRUE;

    if (bReturn)
      bReturn = SetupDiEnumDeviceInterfaces(hDevHandle, 0, guid, iMemberIndex, &deviceInterfaceData) == TRUE;
    else
    {
      bReturn = true;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
        break; // no more USB devices, nothing more to scan
      else
        continue; // try to get other USB devices
    }

    if (bReturn)
    {
      devicedetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
      deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

      BOOL bDetailResult = SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, devicedetailData, nBufferSize , &required, &deviceInfo);
      if (!bDetailResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        free(devicedetailData);
        devicedetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(required * sizeof(TCHAR));
        if (!devicedetailData)
        {
          free(deviceProperty);
          CLog::Log(LOGSEVERE, "%s: memory allocation failed", __FUNCTION__);
          return false;
        }
        devicedetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        nBufferSize = required;
        bDetailResult = SetupDiGetDeviceInterfaceDetail(hDevHandle, &deviceInterfaceData, devicedetailData, nBufferSize , &required, &deviceInfo);
      }

      if (bDetailResult)
      {
        bDetailResult = SetupDiGetDeviceRegistryProperty(hDevHandle, &deviceInfo, SPDRP_HARDWAREID, NULL, (PBYTE)deviceProperty, nPropertyBufferSize, &required);
        if (!bDetailResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
          free(deviceProperty);
          deviceProperty = (char*)malloc(required);
          if (!deviceProperty)
          {
            free(devicedetailData);
            CLog::Log(LOGSEVERE, "%s: memory allocation failed", __FUNCTION__);
            return false;
          }
          nPropertyBufferSize = required;
          bDetailResult = SetupDiGetDeviceRegistryProperty(hDevHandle, &deviceInfo, SPDRP_HARDWAREID, NULL, (PBYTE)deviceProperty, nPropertyBufferSize, &required);
        }
      }

      if (bDetailResult)
      {
        std::string strTmp(deviceProperty);

        StringUtils::ToLower(strTmp);
        size_t posVid, posPid;
        if (((posVid=strTmp.find("\\vid_")) != std::string::npos || (posVid=strTmp.find("&vid_")) != std::string::npos) &&
              ((posPid=strTmp.find("\\pid_")) != std::string::npos || (posPid=strTmp.find("&pid_")) != std::string::npos))
        {
          std::string strVendorId(strTmp, posVid + 5, 4);
          std::string strProductId(strTmp, posPid + 5, 4);
          PeripheralScanResult prevDevice(m_type);
          if (!results.GetDeviceOnLocation(devicedetailData->DevicePath, &prevDevice))
          {
            PeripheralScanResult result(m_type);
            result.m_strLocation  = devicedetailData->DevicePath;
            result.m_iVendorId    = PeripheralTypeTranslator::HexStringToInt(strVendorId.c_str());
            result.m_iProductId   = PeripheralTypeTranslator::HexStringToInt(strProductId.c_str());
            result.m_iSequence    = GetNumberOfPeripheralsWithId(result.m_iVendorId, result.m_iProductId);

            // Assume that buffer is more then enough (we need only 8 chars, initial allocation is 100 chars). If not - just skip type detection.
            if (SetupDiGetDeviceRegistryProperty(hDevHandle, &devInfoData, SPDRP_CLASS, NULL, (PBYTE)deviceProperty, nPropertyBufferSize, &required) &&
                strcmp("HIDClass", deviceProperty) == 0)
              result.m_type = PERIPHERAL_HID;
            else
              result.m_type = defaultType;

            if (!results.ContainsResult(result))
              results.m_results.push_back(result);
          }
        }
      }
    }
  }

  SetupDiDestroyDeviceInfoList(hDevHandle);
  if (devicedetailData)
    free(devicedetailData);
  if (deviceProperty)
    free(deviceProperty);

  return bReturn;
}
