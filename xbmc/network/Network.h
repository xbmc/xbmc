#ifndef NETWORK_H_
#define NETWORK_H_

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

#include <vector>
#include "utils/StdString.h"

enum EncMode { ENC_NONE = 0, ENC_WEP = 1, ENC_WPA = 2, ENC_WPA2 = 3 };
enum NetworkAssignment { NETWORK_DASH = 0, NETWORK_DHCP = 1, NETWORK_STATIC = 2, NETWORK_DISABLED = 3 };

class NetworkAccessPoint
{
public:
   NetworkAccessPoint(CStdString& essId, CStdString& macAddress, int signalStrength, EncMode encryption, int channel = 0)
   {
      m_essId = essId;
      m_macAddress = macAddress;
      m_dBm = signalStrength;
      m_encryptionMode = encryption;
      m_channel = channel;
   }

   CStdString getEssId() { return m_essId; }
   CStdString getMacAddress() { return m_macAddress; }
   int getSignalStrength() { return m_dBm; }
   EncMode getEncryptionMode() { return m_encryptionMode; }
   int getChannel() { return m_channel; }

   /* Returns the quality, normalized as a percentage, of the network access point */
   int getQuality()
   {
      // Cisco dBm lookup table (partially nonlinear)
      // Source: Converting Signal Strength Percentage to dBm Values, 2002
      int quality;
      if (m_dBm >= -10) quality = 100;
      else if (m_dBm >= -20) quality = 85 + (m_dBm + 20);
      else if (m_dBm >= -30) quality = 77 + (m_dBm + 30);
      else if (m_dBm >= -60) quality = 48 + (m_dBm + 60);
      else if (m_dBm >= -98) quality = 13 + (m_dBm + 98);
      else if (m_dBm >= -112) quality = 1 + (m_dBm + 112);
      else quality = 0;
      return quality;
   }

   // Returns a Google Gears specific JSON string
   CStdString toJson()
   {
      CStdString jsonBuffer;
      if (m_macAddress.IsEmpty())
         return "{}";
      jsonBuffer.Format("{\"mac_address\":\"%s\"", m_macAddress.c_str());
      if (!m_essId.IsEmpty())
        jsonBuffer.AppendFormat(",\"ssid\":\"%s\"", m_essId.c_str());
      if (m_dBm < 0)
        jsonBuffer.AppendFormat(",\"signal_strength\":%d", m_dBm);
      if (m_channel != 0)
        jsonBuffer.AppendFormat(",\"channel\":%d", m_channel);
      jsonBuffer.append("}");
      return jsonBuffer;
   }

   // Translates a 802.11a+b frequency into corresponding channel
   static int FreqToChannel(float frequency)
   {
      int IEEE80211Freq[] = {2412, 2417, 2422, 2427, 2432,
                             2437, 2442, 2447, 2452, 2457,
                             2462, 2467, 2472, 2484,
                             5180, 5200, 5210, 5220, 5240, 5250,
                             5260, 5280, 5290, 5300, 5320,
                             5745, 5760, 5765, 5785, 5800, 5805, 5825};
      int IEEE80211Ch[] = {     1,    2,    3,    4,    5,
                                6,    7,    8,    9,   10,
                               11,   12,   13,   14,
                               36,   40,   42,   44,   48,   50,
                               52,   56,   58,   60,   64,
                              149,  152,  153,  157,  160,  161,  165};

      int mod_chan = (int)(frequency / 1000000 + 0.5f);
      for (unsigned int i = 0; i < sizeof(IEEE80211Freq) / sizeof(int); ++i)
      {
         if (IEEE80211Freq[i] == mod_chan)
            return IEEE80211Ch[i];
      }
      return 0; // unknown
   }

private:
   CStdString   m_essId;
   CStdString   m_macAddress;
   int          m_dBm;
   EncMode      m_encryptionMode;
   int          m_channel;
};

class CNetworkInterface
{
public:
   virtual ~CNetworkInterface() {};

   virtual CStdString& GetName(void) = 0;

   virtual bool IsEnabled(void) = 0;
   virtual bool IsConnected(void) = 0;
   virtual bool IsWireless(void) = 0;

   virtual CStdString GetMacAddress(void) = 0;

   virtual CStdString GetCurrentIPAddress() = 0;
   virtual CStdString GetCurrentNetmask() = 0;
   virtual CStdString GetCurrentDefaultGateway(void) = 0;
   virtual CStdString GetCurrentWirelessEssId(void) = 0;

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void) = 0;

   virtual void GetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode) = 0;
   virtual void SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode) = 0;
};



class CNetwork
{
public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN
  };

   CNetwork();
   virtual ~CNetwork();

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void) = 0;
   CNetworkInterface* GetInterfaceByName(CStdString& name);

   // Return the first interface which is active
   virtual CNetworkInterface* GetFirstConnectedInterface(void);

   // Return true if there is a interface for the same network as address
   bool HasInterfaceForIP(unsigned long address);

   // Return true if there's at least one defined network interface
   bool IsAvailable(bool wait = false);

   // Return true if there's at least one interface which is connected
   bool IsConnected(void);

   // Return true if the magic packet was send
   bool WakeOnLan(const char *mac);

   // Get/set the nameserver(s)
   virtual std::vector<CStdString> GetNameServers(void) = 0;
   virtual void SetNameServers(std::vector<CStdString> nameServers) = 0;

   // callback from application controlled thread to handle any setup
   void NetworkMessage(EMESSAGE message, int param);

   void StartServices();
   void StopServices(bool bWait);

   static int ParseHex(char *str, unsigned char *addr);
};
#ifdef HAS_LINUX_NETWORK
#include "linux/NetworkLinux.h"
#else
#include "windows/NetworkWin32.h"
#endif
#endif
