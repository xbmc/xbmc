/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <errno.h>
#include "PlatformDefs.h"
#include "NetworkWin32.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"

// undefine if you want to build without the wlan stuff
// might be needed for VS2003
//#define HAS_WIN32_WLAN_API

#ifdef HAS_WIN32_WLAN_API
#include "Wlanapi.h"
#pragma comment (lib,"Wlanapi.lib")
#endif


using namespace std;

CNetworkInterfaceWin32::CNetworkInterfaceWin32(CNetworkWin32* network, IP_ADAPTER_INFO adapter)

{
   m_network = network;
   m_adapter = adapter;
   m_adaptername = adapter.Description;
}

CNetworkInterfaceWin32::~CNetworkInterfaceWin32(void)
{
}

CStdString& CNetworkInterfaceWin32::GetName(void)
{
  if (!g_charsetConverter.isValidUtf8(m_adaptername))
    g_charsetConverter.unknownToUTF8(m_adaptername);
  return m_adaptername;
}

bool CNetworkInterfaceWin32::IsWireless()
{
  return (m_adapter.Type == IF_TYPE_IEEE80211);
}

bool CNetworkInterfaceWin32::IsEnabled()
{
  return true;
}

bool CNetworkInterfaceWin32::IsConnected()
{
  CStdString strIP = m_adapter.IpAddressList.IpAddress.String;
  return (strIP != "0.0.0.0");
}

CStdString CNetworkInterfaceWin32::GetMacAddress()
{
  CStdString result;
  unsigned char* mAddr = m_adapter.Address;
  result.Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin32::GetMacAddressRaw(char rawMac[6])
{
  memcpy(rawMac, m_adapter.Address, 6);
}

CStdString CNetworkInterfaceWin32::GetCurrentIPAddress(void)
{
  return m_adapter.IpAddressList.IpAddress.String;
}

CStdString CNetworkInterfaceWin32::GetCurrentNetmask(void)
{
  return m_adapter.IpAddressList.IpMask.String;
}

CStdString CNetworkInterfaceWin32::GetCurrentWirelessEssId(void)
{
  CStdString result = "";

#ifdef HAS_WIN32_WLAN_API
  if(IsWireless())
  {
    HANDLE hClientHdl = NULL;
    DWORD dwVersion = 0;
    DWORD dwret = 0;
    PWLAN_CONNECTION_ATTRIBUTES pAttributes;
    DWORD dwSize = 0;

    if(WlanOpenHandle(1,NULL,&dwVersion, &hClientHdl) == ERROR_SUCCESS)
    {
      PWLAN_INTERFACE_INFO_LIST ppInterfaceList;
      if(WlanEnumInterfaces(hClientHdl,NULL, &ppInterfaceList ) == ERROR_SUCCESS)
      {
        for(int i=0; i<ppInterfaceList->dwNumberOfItems;i++)
        {
          GUID guid = ppInterfaceList->InterfaceInfo[i].InterfaceGuid;
          WCHAR wcguid[64];
          StringFromGUID2(guid, (LPOLESTR)&wcguid, 64);
          CStdStringW strGuid = wcguid;
          CStdStringW strAdaptername = m_adapter.AdapterName;
          if( strGuid == strAdaptername)
          {
            if(WlanQueryInterface(hClientHdl,&ppInterfaceList->InterfaceInfo[i].InterfaceGuid,wlan_intf_opcode_current_connection, NULL, &dwSize, (PVOID*)&pAttributes, NULL ) == ERROR_SUCCESS)
            {
              result = (char*)pAttributes->wlanAssociationAttributes.dot11Ssid.ucSSID;
              WlanFreeMemory((PVOID*)&pAttributes);
            }
            else
              OutputDebugString("Can't query wlan interface\n");
          }
        }
      }
      WlanCloseHandle(&hClientHdl, NULL);
    }
    else
      OutputDebugString("Can't open wlan handle\n");
  }
#endif
  return result;
}

CStdString CNetworkInterfaceWin32::GetCurrentDefaultGateway(void)
{
  return m_adapter.GatewayList.IpAddress.String;
}

CNetworkWin32::CNetworkWin32(void)
{
  queryInterfaceList();
}

CNetworkWin32::~CNetworkWin32(void)
{
  CleanInterfaceList();
  m_netrefreshTimer.Stop();
}

void CNetworkWin32::CleanInterfaceList()
{
  vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkWin32::GetInterfaceList(void)
{
  CSingleLock lock (m_critSection);
  if(m_netrefreshTimer.GetElapsedSeconds() >= 5.0f)
    queryInterfaceList();

  return m_interfaces;
}

void CNetworkWin32::queryInterfaceList()
{
  CleanInterfaceList();
  m_netrefreshTimer.StartZero();

  PIP_ADAPTER_INFO adapterInfo;
  PIP_ADAPTER_INFO adapter = NULL;

  ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);

  adapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
  if (adapterInfo == NULL)
    return;

  if (GetAdaptersInfo(adapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(adapterInfo);
    adapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
    if (adapterInfo == NULL)
    {
      OutputDebugString("Error allocating memory needed to call GetAdaptersinfo\n");
      return;
    }
  }

  if ((GetAdaptersInfo(adapterInfo, &ulOutBufLen)) == NO_ERROR)
  {
    adapter = adapterInfo;
    while (adapter)
    {
      m_interfaces.push_back(new CNetworkInterfaceWin32(this, *adapter));
      adapter = adapter->Next;
    }
  }
  free(adapterInfo);
}

std::vector<CStdString> CNetworkWin32::GetNameServers(void)
{
  std::vector<CStdString> result;

  FIXED_INFO *pFixedInfo;
  ULONG ulOutBufLen;
  IP_ADDR_STRING *pIPAddr;

  pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
  if (pFixedInfo == NULL)
  {
    OutputDebugString("Error allocating memory needed to call GetNetworkParams\n");
    return result;
  }
  ulOutBufLen = sizeof (FIXED_INFO);
  if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(pFixedInfo);
    pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
    if (pFixedInfo == NULL)
    {
      OutputDebugString("Error allocating memory needed to call GetNetworkParams\n");
      return result;
    }
  }

  if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR)
  {
    result.push_back(pFixedInfo->DnsServerList.IpAddress.String);
    pIPAddr = pFixedInfo->DnsServerList.Next;
    while(pIPAddr)
    {
      result.push_back(pIPAddr->IpAddress.String);
      pIPAddr = pIPAddr->Next;
    }

  }
  free(pFixedInfo);

  return result;
}

void CNetworkWin32::SetNameServers(std::vector<CStdString> nameServers)
{
  return;
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin32::GetAccessPoints(void)
{
   std::vector<NetworkAccessPoint> result;

   return result;
}

void CNetworkInterfaceWin32::GetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
  ipAddress = "0.0.0.0";
  networkMask = "0.0.0.0";
  defaultGateway = "0.0.0.0";
  essId = "";
  key = "";
  encryptionMode = ENC_NONE;
  assignment = NETWORK_DISABLED;


  PIP_ADAPTER_INFO adapterInfo;
  PIP_ADAPTER_INFO adapter = NULL;

  ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);

  adapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
  if (adapterInfo == NULL)
    return;

  if (GetAdaptersInfo(adapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(adapterInfo);
    adapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
    if (adapterInfo == NULL)
    {
      OutputDebugString("Error allocating memory needed to call GetAdaptersinfo\n");
      return;
    }
  }

  if ((GetAdaptersInfo(adapterInfo, &ulOutBufLen)) == NO_ERROR)
  {
    adapter = adapterInfo;
    while (adapter)
    {
      if(m_adapter.Index == adapter->Index)
      {
        ipAddress = adapter->IpAddressList.IpAddress.String;
        networkMask = adapter->IpAddressList.IpMask.String;
        defaultGateway = adapter->GatewayList.IpAddress.String;
        if (adapter->DhcpEnabled)
          assignment = NETWORK_DHCP;
        else
          assignment = NETWORK_STATIC;

      }
      adapter = adapter->Next;
    }
  }
  free(adapterInfo);

#ifdef HAS_WIN32_WLAN_API
  if(IsWireless())
  {
    HANDLE hClientHdl = NULL;
    DWORD dwVersion = 0;
    DWORD dwret = 0;
    PWLAN_CONNECTION_ATTRIBUTES pAttributes;
    DWORD dwSize = 0;

    if(WlanOpenHandle(1,NULL,&dwVersion, &hClientHdl) == ERROR_SUCCESS)
    {
      PWLAN_INTERFACE_INFO_LIST ppInterfaceList;
      if(WlanEnumInterfaces(hClientHdl,NULL, &ppInterfaceList ) == ERROR_SUCCESS)
      {
        for(int i=0; i<ppInterfaceList->dwNumberOfItems;i++)
        {
          GUID guid = ppInterfaceList->InterfaceInfo[i].InterfaceGuid;
          WCHAR wcguid[64];
          StringFromGUID2(guid, (LPOLESTR)&wcguid, 64);
          CStdStringW strGuid = wcguid;
          CStdStringW strAdaptername = m_adapter.AdapterName;
          if( strGuid == strAdaptername)
          {
            if(WlanQueryInterface(hClientHdl,&ppInterfaceList->InterfaceInfo[i].InterfaceGuid,wlan_intf_opcode_current_connection, NULL, &dwSize, (PVOID*)&pAttributes, NULL ) == ERROR_SUCCESS)
            {
              essId = (char*)pAttributes->wlanAssociationAttributes.dot11Ssid.ucSSID;
              if(pAttributes->wlanSecurityAttributes.bSecurityEnabled)
              {
                switch(pAttributes->wlanSecurityAttributes.dot11AuthAlgorithm)
                {
                case DOT11_AUTH_ALGO_80211_SHARED_KEY:
                  encryptionMode = ENC_WEP;
                  break;
                case DOT11_AUTH_ALGO_WPA:
                case DOT11_AUTH_ALGO_WPA_PSK:
                  encryptionMode = ENC_WPA;
                  break;
                case DOT11_AUTH_ALGO_RSNA:
                case DOT11_AUTH_ALGO_RSNA_PSK:
                  encryptionMode = ENC_WPA2;
                }
              }
              WlanFreeMemory((PVOID*)&pAttributes);
            }
            else
              OutputDebugString("Can't query wlan interface\n");
          }
        }
      }
      WlanCloseHandle(&hClientHdl, NULL);
    }
    else
      OutputDebugString("Can't open wlan handle\n");
  }
  // Todo: get the key (WlanGetProfile, CryptUnprotectData?)
#endif
}

void CNetworkInterfaceWin32::SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
  return;
}

void CNetworkInterfaceWin32::WriteSettings(FILE* fw, NetworkAssignment assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
  return;
}
