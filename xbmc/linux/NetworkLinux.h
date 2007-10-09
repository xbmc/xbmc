#ifndef NETWORK_LINUX_H_
#define NETWORK_LINUX_H_

#include <vector>
#include "StdString.h"
#include "Network.h"

class CNetworkLinux;

class CNetworkInterfaceLinux : public CNetworkInterface
{
public:
   CNetworkInterfaceLinux(CNetworkLinux* network, CStdString interfaceName);
   ~CNetworkInterfaceLinux(void);
   
   virtual CStdString& GetName(void);
   
   virtual bool IsEnabled(void);
   virtual bool IsConnected(void);
   virtual bool IsWireless(void);

   virtual CStdString GetMacAddress(void);

   virtual CStdString GetCurrentIPAddress();
   virtual CStdString GetCurrentNetmask();
   virtual CStdString GetCurrentDefaultGateway(void);
   virtual CStdString GetCurrentWirelessEssId(void);

   virtual void GetSettings(bool& isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, bool& keyIsString);
   virtual void SetSettings(bool isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, bool keyIsString);

private:   
   CStdString     m_interfaceName;
   CNetworkLinux* m_network;
};

class CNetworkLinux : public CNetwork   
{
public:   
   CNetworkLinux(void);
   virtual ~CNetworkLinux(void);
   
   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void);

   // Get/set the nameserver(s)
   virtual std::vector<CStdString> GetNameServers(void);
   virtual void SetNameServers(std::vector<CStdString> nameServers);
   
   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void);

   friend class CNetworkInterfaceLinux;
        
private:
   int GetSocket() { return m_sock; }
   void queryInterfaceList();   
   std::vector<CNetworkInterface*> m_interfaces;
   int m_sock;
};

#endif
