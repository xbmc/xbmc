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

#include <errno.h>
#include <iphlpapi.h>
#include <IcmpAPI.h>
#include "PlatformDefs.h"
#include "NetworkWin32.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "win32/WIN32Util.h"

// undefine if you want to build without the wlan stuff
// might be needed for VS2003
#define HAS_WIN32_WLAN_API

#ifdef HAS_WIN32_WLAN_API
#include "Wlanapi.h"
#pragma comment (lib,"Wlanapi.lib")
#endif


using namespace std;

CNetworkInterfaceWin32::CNetworkInterfaceWin32(CNetworkWin32* network, IP_ADAPTER_INFO adapter):
   m_adaptername(adapter.Description)
{
   m_network = network;
   m_adapter = adapter;
}

CNetworkInterfaceWin32::~CNetworkInterfaceWin32(void)
{
}

std::string& CNetworkInterfaceWin32::GetName(void)
{
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
  std::string strIP = m_adapter.IpAddressList.IpAddress.String;
  return (strIP != "0.0.0.0");
}

std::string CNetworkInterfaceWin32::GetMacAddress()
{
  std::string result;
  unsigned char* mAddr = m_adapter.Address;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin32::GetMacAddressRaw(char rawMac[6])
{
  memcpy(rawMac, m_adapter.Address, 6);
}

std::string CNetworkInterfaceWin32::GetCurrentIPAddress(void)
{
  return m_adapter.IpAddressList.IpAddress.String;
}

std::string CNetworkInterfaceWin32::GetCurrentNetmask(void)
{
  return m_adapter.IpAddressList.IpMask.String;
}

std::string CNetworkInterfaceWin32::GetCurrentWirelessEssId(void)
{
  std::string result = "";

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
        for(unsigned int i=0; i<ppInterfaceList->dwNumberOfItems;i++)
        {
          GUID guid = ppInterfaceList->InterfaceInfo[i].InterfaceGuid;
          WCHAR wcguid[64];
          StringFromGUID2(guid, (LPOLESTR)&wcguid, 64);
          std::wstring strGuid = wcguid;
          std::wstring strAdaptername;
          g_charsetConverter.utf8ToW(m_adapter.AdapterName, strAdaptername);
          if( strGuid == strAdaptername)
          {
            if(WlanQueryInterface(hClientHdl,&ppInterfaceList->InterfaceInfo[i].InterfaceGuid,wlan_intf_opcode_current_connection, NULL, &dwSize, (PVOID*)&pAttributes, NULL ) == ERROR_SUCCESS)
            {
              result = (char*)pAttributes->wlanAssociationAttributes.dot11Ssid.ucSSID;
              WlanFreeMemory((PVOID*)&pAttributes);
            }
            else
              CLog::Log(LOGERROR, "%s: Can't query wlan interface", __FUNCTION__);
          }
        }
      }
      WlanCloseHandle(&hClientHdl, NULL);
    }
    else
      CLog::Log(LOGERROR, "%s: Can't open wlan handle", __FUNCTION__);
  }
#endif
  return result;
}

std::string CNetworkInterfaceWin32::GetCurrentDefaultGateway(void)
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
      return;
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

std::vector<std::string> CNetworkWin32::GetNameServers(void)
{
  std::vector<std::string> result;

  FIXED_INFO *pFixedInfo;
  ULONG ulOutBufLen;
  IP_ADDR_STRING *pIPAddr;

  pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
  if (pFixedInfo == NULL)
    return result;

  ulOutBufLen = sizeof (FIXED_INFO);
  if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(pFixedInfo);
    pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
    if (pFixedInfo == NULL)
      return result;
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

void CNetworkWin32::SetNameServers(const std::vector<std::string>& nameServers)
{
  return;
}

bool CNetworkWin32::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  char SendData[]    = "poke";
  HANDLE hIcmpFile   = IcmpCreateFile();
  BYTE ReplyBuffer [sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];

  SetLastError(ERROR_SUCCESS);

  DWORD dwRetVal = IcmpSendEcho(hIcmpFile, host, SendData, sizeof(SendData), 
                                NULL, ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);

  DWORD lastErr = GetLastError();
  if (lastErr != ERROR_SUCCESS && lastErr != IP_REQ_TIMED_OUT)
    CLog::Log(LOGERROR, "%s - IcmpSendEcho failed - %s", __FUNCTION__, CWIN32Util::WUSysMsg(lastErr).c_str());

  IcmpCloseHandle (hIcmpFile);

  if (dwRetVal != 0)
  {
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
    return (pEchoReply->Status == IP_SUCCESS);
  }
  return false;
}

bool CNetworkInterfaceWin32::GetHostMacAddress(unsigned long host, std::string& mac)
{
  IPAddr src_ip = inet_addr(GetCurrentIPAddress().c_str());
  BYTE bPhysAddr[6];      // for 6-byte hardware addresses
  ULONG PhysAddrLen = 6;  // default to length of six bytes

  memset(&bPhysAddr, 0xff, sizeof (bPhysAddr));

  DWORD dwRetVal = SendARP(host, src_ip, &bPhysAddr, &PhysAddrLen);
  if (dwRetVal == NO_ERROR)
  {
    if (PhysAddrLen == 6)
    {
      mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", 
        bPhysAddr[0], bPhysAddr[1], bPhysAddr[2], 
        bPhysAddr[3], bPhysAddr[4], bPhysAddr[5]);
      return true;
    }
    else
      CLog::Log(LOGERROR, "%s - SendArp completed successfully, but mac address has length != 6 (%d)", __FUNCTION__, PhysAddrLen);
  }
  else
    CLog::Log(LOGERROR, "%s - SendArp failed with error (%d)", __FUNCTION__, dwRetVal);

  return false;
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin32::GetAccessPoints(void)
{
   std::vector<NetworkAccessPoint> result;
#ifdef HAS_WIN32_WLAN_API
  if (!IsWireless())
    return result;

  // According to Mozilla: "We could be executing on either Windows XP or Windows
  // Vista, so use the lower version of the client WLAN API. It seems that the
  // negotiated version is the Vista version irrespective of what we pass!"
  // http://dxr.mozilla.org/mozilla-central/source/netwerk/wifi/nsWifiScannerWin.cpp#l51
  static const int xpWlanClientVersion = 1;
  DWORD negotiated_version;
  DWORD dwResult;
  HANDLE wlan_handle = NULL;

  // Get the handle to the WLAN API
  dwResult = WlanOpenHandle(xpWlanClientVersion, NULL, &negotiated_version, &wlan_handle);
  if (dwResult != ERROR_SUCCESS || !wlan_handle)
  {
    CLog::Log(LOGERROR, "Could not load the client WLAN API");
    return result;
  }

  // Get the list of interfaces (WlanEnumInterfaces allocates interface_list)
  WLAN_INTERFACE_INFO_LIST *interface_list = NULL;
  dwResult = WlanEnumInterfaces(wlan_handle, NULL, &interface_list);
  if (dwResult != ERROR_SUCCESS || !interface_list)
  {
    WlanCloseHandle(wlan_handle, NULL);
    CLog::Log(LOGERROR, "Failed to get the list of interfaces");
    return result;
  }

  for (unsigned int i = 0; i < interface_list->dwNumberOfItems; ++i)
  {
    GUID guid = interface_list->InterfaceInfo[i].InterfaceGuid;
    WCHAR wcguid[64];
    StringFromGUID2(guid, (LPOLESTR)&wcguid, 64);
    std::wstring strGuid = wcguid;
    std::wstring strAdaptername;
    g_charsetConverter.utf8ToW(m_adapter.AdapterName, strAdaptername);
    if (strGuid == strAdaptername)
    {
      WLAN_BSS_LIST *bss_list;
      HRESULT rv = WlanGetNetworkBssList(wlan_handle,
                                         &interface_list->InterfaceInfo[i].InterfaceGuid,
                                         NULL,               // Get all SSIDs
                                         dot11_BSS_type_any, // unused
                                         false,              // bSecurityEnabled - unused
                                         NULL,               // reserved
                                         &bss_list);
      if (rv != ERROR_SUCCESS || !bss_list)
        break;
      for (unsigned int j = 0; j < bss_list->dwNumberOfItems; ++j)
      {
        const WLAN_BSS_ENTRY bss_entry = bss_list->wlanBssEntries[j];
        // Add the access point info to the list of results
        std::string essId((char*)bss_entry.dot11Ssid.ucSSID, (unsigned int)bss_entry.dot11Ssid.uSSIDLength);
        std::string macAddress;
        // macAddress is big-endian, write in byte chunks
        macAddress = StringUtils::Format("%02x-%02x-%02x-%02x-%02x-%02x",
          bss_entry.dot11Bssid[0], bss_entry.dot11Bssid[1], bss_entry.dot11Bssid[2],
          bss_entry.dot11Bssid[3], bss_entry.dot11Bssid[4], bss_entry.dot11Bssid[5]);
        int signalLevel = bss_entry.lRssi;
        EncMode encryption = ENC_NONE; // TODO
        int channel = NetworkAccessPoint::FreqToChannel((float)bss_entry.ulChCenterFrequency * 1000);
        result.push_back(NetworkAccessPoint(essId, macAddress, signalLevel, encryption, channel));
      }
      WlanFreeMemory(bss_list);
      break;
    }
  }

  // Free the interface list
  WlanFreeMemory(interface_list);

  // Close the handle
  WlanCloseHandle(wlan_handle, NULL);

#endif

  return result;
}

void CNetworkInterfaceWin32::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
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
      return;
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
        for(unsigned int i=0; i<ppInterfaceList->dwNumberOfItems;i++)
        {
          GUID guid = ppInterfaceList->InterfaceInfo[i].InterfaceGuid;
          WCHAR wcguid[64];
          StringFromGUID2(guid, (LPOLESTR)&wcguid, 64);
          std::wstring strGuid = wcguid;
          std::wstring strAdaptername;
          g_charsetConverter.utf8ToW(m_adapter.AdapterName, strAdaptername);
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
              CLog::Log(LOGERROR, "%s: Can't query wlan interface", __FUNCTION__);
          }
        }
      }
      WlanCloseHandle(&hClientHdl, NULL);
    }
    else
      CLog::Log(LOGERROR, "%s: Can't open wlan handle", __FUNCTION__);
  }
  // Todo: get the key (WlanGetProfile, CryptUnprotectData?)
#endif
}

void CNetworkInterfaceWin32::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
  return;
}

void CNetworkInterfaceWin32::WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
  return;
}
