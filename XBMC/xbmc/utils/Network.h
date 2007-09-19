#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>
#include "StdString.h"

class NetworkAccessPoint
{
public:
   NetworkAccessPoint(CStdString& essId, int quality, bool encrypted)
   {
      m_essId = essId;
      m_quality = quality;
      m_isEncrypted = encrypted;
   }

   CStdString getEssId() { return m_essId; }
   int getQuality() { return m_quality; } 
   bool isEncrypted() { return m_isEncrypted; } 
   
private:
   CStdString   m_essId;
   int          m_quality;
   bool         m_isEncrypted;
};

class CNetworkInterface
{
public:
   virtual CStdString& GetName(void) = 0;
   
   virtual bool IsEnabled(void) = 0;
   virtual bool IsConnected(void) = 0;
   virtual bool IsWireless(void) = 0;

   virtual CStdString GetMacAddress(void) = 0;

   virtual CStdString GetCurrentIPAddress() = 0;
   virtual CStdString GetCurrentNetmask() = 0;
   virtual CStdString GetCurrentDefaultGateway(void) = 0;
   virtual void GetSettingsIP(bool& isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway) = 0;
   virtual void SetSettingsIP(bool isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway) = 0;
   
   virtual CStdString GetCurrentWirelessEssId(void) = 0;
   virtual void GetSettingsWireless(CStdString& essId, CStdString& key, bool& keyIsString) = 0;
   virtual void SetSettingsWireless(CStdString& essId, CStdString& key, bool keyIsString) = 0;
};

class CNetwork
{
public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN,
  };

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void) = 0;
   
   // Return the first interface which is active
   CNetworkInterface* GetFirstConnectedInterface(void);
   
   // Return true if there's at least one defined network interface
   bool IsAvailable(void);
   
   // Return true if there's at least one interface which is connected
   bool IsConnected(void);

   // Get/set the nameserver(s)
   virtual std::vector<CStdString> GetNameServers(void) = 0;
   virtual void SetNameServers(std::vector<CStdString> nameServers) = 0;
   
   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void) = 0;
      
   // callback from application controlled thread to handle any setup
   void NetworkMessage(EMESSAGE message, DWORD dwParam);
   
   static int ParseHex(char *str, unsigned char *addr);
};

#ifdef HAS_XBOX_NETWORK
#include "xbox/NetworkXbox.h"
#endif

#ifdef _LINUX
#include "linux/NetworkLinux.h"
#endif

#endif
