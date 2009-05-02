/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <errno.h>
#include "PlatformDefs.h"
#include "NetworkWin32.h"
#include "Util.h"
#include "log.h"

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
#ifdef _LINUX
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
#else
  return true;
#endif
}

bool CNetworkInterfaceWin32::IsConnected()
{
  CStdString strIP = m_adapter.IpAddressList.IpAddress.String;
  return (strIP != "0.0.0.0");
}

CStdString CNetworkInterfaceWin32::GetMacAddress()
{
  CStdString result = "";
  result = CStdString((char*)m_adapter.Address);
  return result;
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
  return m_interfaces;
}

void CNetworkWin32::queryInterfaceList()
{
  m_interfaces.clear();

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
   FILE* fp = fopen("/etc/resolv.conf", "w");
   if (fp != NULL)
   {
      for (unsigned int i = 0; i < nameServers.size(); i++)
      {
         fprintf(fp, "nameserver %s\n", nameServers[i].c_str());
      }
      fclose(fp);
   }
   else
   {
      // TODO:
   }
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin32::GetAccessPoints(void)
{
   std::vector<NetworkAccessPoint> result;

   if (!IsWireless())
      return result;

#ifdef _LINUX
   // Query the wireless extentsions version number. It will help us when we
   // parse the resulting events
   struct iwreq iwr;
   char rangebuffer[sizeof(iw_range) * 2];    /* Large enough */
   struct iw_range*  range = (struct iw_range*) rangebuffer;

   memset(rangebuffer, 0, sizeof(rangebuffer));
   iwr.u.data.pointer = (caddr_t) rangebuffer;
   iwr.u.data.length = sizeof(rangebuffer);
   iwr.u.data.flags = 0;
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   if (ioctl(m_network->GetSocket(), SIOCGIWRANGE, &iwr) < 0)
   {
      CLog::Log(LOGWARNING, "%-8.16s  Driver has no Wireless Extension version information.",
         GetName().c_str());
      return result;
   }

   // Scan for wireless access points
   memset(&iwr, 0, sizeof(iwr));
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   if (ioctl(m_network->GetSocket(), SIOCSIWSCAN, &iwr) < 0)
   {
      CLog::Log(LOGWARNING, "Cannot initiate wireless scan: ioctl[SIOCSIWSCAN]: %s", strerror(errno));
      return result;
   }

   // Get the results of the scanning. Three scenarios:
   //    1. There's not enough room in the result buffer (E2BIG)
   //    2. The scanning is not complete (EAGAIN) and we need to try again. We cap this with 15 seconds.
   //    3. Were'e good.
   int duration = 0; // ms
   unsigned char* res_buf = NULL;
   int res_buf_len = IW_SCAN_MAX_DATA;
   while (duration < 15000)
   {
      if (!res_buf)
         res_buf = (unsigned char*) malloc(res_buf_len);

      if (res_buf == NULL)
      {
         CLog::Log(LOGWARNING, "Cannot alloc memory for wireless scanning");
         return result;
      }

      strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
      iwr.u.data.pointer = res_buf;
      iwr.u.data.length = res_buf_len;
      iwr.u.data.flags = 0;
      int x = ioctl(m_network->GetSocket(), SIOCGIWSCAN, &iwr);
      if (x == 0)
         break;

      if (errno == E2BIG && res_buf_len < 100000)
      {
         free(res_buf);
         res_buf = NULL;
         res_buf_len *= 2;
         CLog::Log(LOGDEBUG, "Scan results did not fit - trying larger buffer (%lu bytes)",
                        (unsigned long) res_buf_len);
      }
      else if (errno == EAGAIN)
      {
         usleep(250000); // sleep for 250ms
         duration += 250;
      }
      else
      {
         CLog::Log(LOGWARNING, "Cannot get wireless scan results: ioctl[SIOCGIWSCAN]: %s", strerror(errno));
         free(res_buf);
         return result;
      }
   }

   size_t len = iwr.u.data.length;
   char* pos = (char *) res_buf;
   char* end = (char *) res_buf + len;
   char* custom;
   struct iw_event iwe_buf, *iwe = &iwe_buf;

   CStdString essId;
   int quality = 0;
   EncMode encryption = ENC_NONE;
   bool first = true;

   while (pos + IW_EV_LCP_LEN <= end)
   {
      /* Event data may be unaligned, so make a local, aligned copy
       * before processing. */
      memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
      if (iwe->len <= IW_EV_LCP_LEN)
         break;

      custom = pos + IW_EV_POINT_LEN;
      if (range->we_version_compiled > 18 &&
          (iwe->cmd == SIOCGIWESSID ||
           iwe->cmd == SIOCGIWENCODE ||
           iwe->cmd == IWEVGENIE ||
           iwe->cmd == IWEVCUSTOM))
      {
         /* Wireless extentsions v19 removed the pointer from struct iw_point */
         char *dpos = (char *) &iwe_buf.u.data.length;
         int dlen = dpos - (char *) &iwe_buf;
         memcpy(dpos, pos + IW_EV_LCP_LEN, sizeof(struct iw_event) - dlen);
      }
      else
      {
         memcpy(&iwe_buf, pos, sizeof(struct iw_event));
         custom += IW_EV_POINT_OFF;
      }

      switch (iwe->cmd)
      {
         case SIOCGIWAP:
            if (first)
               first = false;
            else
               result.push_back(NetworkAccessPoint(essId, quality, encryption));
               encryption = ENC_NONE;
            break;

         case SIOCGIWESSID:
         {
            char essid[IW_ESSID_MAX_SIZE+1];
            memset(essid, '\0', sizeof(essid));
            if ((custom) && (iwe->u.essid.length))
            {
               memcpy(essid, custom, iwe->u.essid.length);
               essId = essid;
            }
            break;
         }

         case IWEVQUAL:
             quality = iwe->u.qual.qual;
             break;

         case SIOCGIWENCODE:
             if (!(iwe->u.data.flags & IW_ENCODE_DISABLED) && encryption == ENC_NONE)
                encryption = ENC_WEP;
             break;

         case IWEVGENIE:
         {
            int offset = 0;
            while (offset <= iwe_buf.u.data.length)
            {
               switch ((unsigned char)custom[offset])
               {
                  case 0xdd: /* WPA1 */
                     if (encryption != ENC_WPA2)
                        encryption = ENC_WPA;
                     break;
                  case 0x30: /* WPA2 */
                     encryption = ENC_WPA2;
               }

               offset += custom[offset+1] + 2;
            }
         }
      }

      pos += iwe->len;
   }

   if (!first)
      result.push_back(NetworkAccessPoint(essId, quality, encryption));

   free(res_buf);
   res_buf = NULL;
#endif

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
  if(IsWireless())
  {
  }
  else
  {
    /*if(assignment == NETWORK_STATIC)
    {
      DWORD dwRet = 0;
      ULONG NTEContext = 0;  
      ULONG NTEInstance;  

      if((dwRet = AddIPAddress(inet_addr(ipAddress.c_str()), inet_addr(networkMask.c_str()), m_adapter.Index, &NTEContext, &NTEInstance)) == NO_ERROR)
      {
        if((dwRet = DeleteIPAddress(m_adapter.IpAddressList.Context)) != NO_ERROR)
          CLog::Log(LOGERROR, "Unable to delete IP entry: %s, Error code: %d",m_adapter.IpAddressList.IpAddress.String, dwRet);
      }
      else
        CLog::Log(LOGERROR, "Unable to add IP entry: %s, Error code: %d", ipAddress.c_str(),dwRet);
    }*/
  }

}

void CNetworkInterfaceWin32::WriteSettings(FILE* fw, NetworkAssignment assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
#ifdef _LINUX
   if (assignment == NETWORK_DHCP)
   {
      fprintf(fw, "iface %s inet dhcp\n", GetName().c_str());
   }
   else if (assignment == NETWORK_STATIC)
   {
      fprintf(fw, "iface %s inet static\n", GetName().c_str());
      fprintf(fw, "  address %s\n", ipAddress.c_str());
      fprintf(fw, "  netmask %s\n", networkMask.c_str());
      fprintf(fw, "  gateway %s\n", defaultGateway.c_str());
   }

   if (assignment != NETWORK_DISABLED && IsWireless())
   {
      if (encryptionMode == ENC_NONE)
      {
         fprintf(fw, "  wireless-essid %s\n", essId.c_str());
      }
      else if (encryptionMode == ENC_WEP)
      {
         fprintf(fw, "  wireless-essid %s\n", essId.c_str());
         fprintf(fw, "  wireless-key s:%s\n", key.c_str());
      }
      else if (encryptionMode == ENC_WPA || encryptionMode == ENC_WPA2)
      {
         fprintf(fw, "  wpa-ssid %s\n", essId.c_str());
         fprintf(fw, "  wpa-psk %s\n", key.c_str());
         fprintf(fw, "  wpa-proto %s\n", encryptionMode == ENC_WPA ? "WPA" : "WPA2");
      }
   }

   if (assignment != NETWORK_DISABLED)
      fprintf(fw, "auto %s\n\n", GetName().c_str());
#endif
}