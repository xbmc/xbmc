#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <linux/sockios.h>
#include <errno.h>
#include <resolv.h>
#include "PlatformDefs.h"
#include "NetworkLinux.h"

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkLinux* network, CStdString interfaceName) 
      
{
   m_network = network;
   m_interfaceName = interfaceName;
}

CNetworkInterfaceLinux::~CNetworkInterfaceLinux(void)
{
}

CStdString& CNetworkInterfaceLinux::GetName(void) 
{ 
   return m_interfaceName; 
}

bool CNetworkInterfaceLinux::IsWireless()
{
	struct iwreq wrq;      
   strcpy(wrq.ifr_name, m_interfaceName.c_str()); 
   if (ioctl(m_network->GetSocket(), SIOCGIWNAME, &wrq) < 0)
    	return false;

   return true;
}

bool CNetworkInterfaceLinux::IsEnabled()
{
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return (ifr.ifr_flags & IFF_UP == IFF_UP);
}

bool CNetworkInterfaceLinux::IsConnected()
{
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return (ifr.ifr_flags & IFF_RUNNING == IFF_RUNNING);
}

CStdString CNetworkInterfaceLinux::GetMacAddress()
{
   CStdString result = "";
   
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
   {
      result.Format("%hhX:%hhX:%hhX:%hhX:%hhX:%hhX", 
         ifr.ifr_hwaddr.sa_data[0], 
         ifr.ifr_hwaddr.sa_data[1], 
         ifr.ifr_hwaddr.sa_data[2], 
         ifr.ifr_hwaddr.sa_data[3], 
         ifr.ifr_hwaddr.sa_data[4], 
         ifr.ifr_hwaddr.sa_data[5]);
   } 

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentIPAddress(void)
{
   CStdString result = "";
   
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0)
   {
      result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
   } 

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentNetmask(void)
{
   CStdString result = "";
   
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFNETMASK, &ifr) >= 0)
   {
      result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
   } 

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentWirelessEssId(void)
{
   CStdString result = "";
   
   char essid[IW_ESSID_MAX_SIZE + 1];
   essid[0] = '\0';
   
   struct iwreq wrq;
   strcpy(wrq.ifr_name,  m_interfaceName.c_str());
   wrq.u.essid.pointer = (caddr_t) essid;
   wrq.u.essid.length = 0;
   wrq.u.essid.flags = 0;
   if (ioctl(m_network->GetSocket(), SIOCGIWESSID, &wrq) >= 0)
   {
      result = essid;
   }
   
   return result;   
}

CStdString CNetworkInterfaceLinux::GetCurrentDefaultGateway(void)
{
   CStdString result = "";
   
   FILE* fp = fopen("/proc/net/route", "r");
   if (!fp)
   {
     // TBD: Error
     return result;
   }
   
   char* line = NULL;
   char iface[16];
   char dst[128];
   char gateway[128];
   size_t linel = 0;
   int n;
   char* p;
   int linenum = 0;
   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      // skip first two lines
      if (linenum++ < 1)
         continue;
   	
   	// search where the word begins	
      n = sscanf(line,  "%16s %128s %128s",
         iface, dst, gateway);
         
      if (n < 3)
         continue;
      
      if (strcmp(iface, m_interfaceName.c_str()) == 0 &&
          strcmp(dst, "00000000") == 0 && 
          strcmp(gateway, "00000000") != 0)
      {
         unsigned char gatewayAddr[4];
         int len = CNetwork::ParseHex(gateway, gatewayAddr);
         if (len = 4)
         {
            struct in_addr in;
            in.s_addr = (gatewayAddr[0] << 24) | (gatewayAddr[1] << 16) | 
                        (gatewayAddr[2] << 8) | (gatewayAddr[3]);
            result = inet_ntoa(in);                    
            break;
         }
      }
   }
   
   fclose(fp);
   
   return result;
}

CNetworkLinux::CNetworkLinux(void)
{
   m_sock = socket(AF_INET, SOCK_DGRAM, 0);
   queryInterfaceList();   
}

CNetworkLinux::~CNetworkLinux(void)
{
   if (m_sock != -1)
      close(CNetworkLinux::m_sock); 
}

std::vector<CNetworkInterface*>& CNetworkLinux::GetInterfaceList(void)
{
   return m_interfaces;
}

void CNetworkLinux::queryInterfaceList()
{
   m_interfaces.clear();
         
   FILE* fp = fopen("/proc/net/dev", "r");
   if (!fp)
   {
     // TBD: Error
     return;
   }
   
   char* line = NULL;
   size_t linel = 0;
   int n;
   char* p;
   int linenum = 0;
   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      // skip first two lines
      if (linenum++ < 2)
         continue;
   	
   	// search where the word begins	
      p = line;
      while (isspace(*p))
   		++p;
      
      // read word until :
   	n = strcspn(p, ": \t");
   	p[n] = 0;
   	
   	// ignore localhost device
   	if (strcmp(p, "lo") == 0)
   	   continue;

      // save the result   	   
      CStdString interfaceName = p;
   	m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName));
   }
   
   fclose(fp);
}

std::vector<CStdString> CNetworkLinux::GetNameServers(void)
{
   std::vector<CStdString> result;
   
   res_init();
   
   for (int i = 0; i < _res.nscount; i ++)
   {
      CStdString ns = inet_ntoa(((struct sockaddr_in *)&_res.nsaddr_list[0])->sin_addr);
      result.push_back(ns);
   }
      
   return result;
}

void CNetworkLinux::SetNameServers(std::vector<CStdString> nameServers)
{
}
   
std::vector<NetworkAccessPoint>  CNetworkLinux::GetAccessPoints(void)
{
   
}
  
void CNetworkInterfaceLinux::GetSettingsIP(bool& isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway)
{
   if (GetName().Equals("eth0"))
   {
	   isDHCP = false;
	   ipAddress = "10.10.10.10";
	   networkMask = "11.11.11.11";
	   defaultGateway = "12.12.12.12";
	}
	else
	{
	   isDHCP = true;
	   ipAddress = "0.0.0.0";
	   networkMask = "0.0.0.0";
	   defaultGateway = "0.0.0.0";
	}
}

void CNetworkInterfaceLinux::SetSettingsIP(bool isDHCP, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway)
{
}
   
void CNetworkInterfaceLinux::GetSettingsWireless(CStdString& essId, CStdString& key, bool& keyIsString)
{
	essId = "tals";
	key = "momo";
	keyIsString = true;	
}

void CNetworkInterfaceLinux::SetSettingsWireless(CStdString& essId, CStdString& key, bool keyIsString)
{
}
     
CNetworkLinux g_network;
 
   /*
int main(void)
{
  CStdString mac;
  CNetworkLinux x;
  x.GetNameServers();
  std::vector<CNetworkInterface*>& ifaces = x.GetInterfaceList();
  CNetworkInterface* i1 = ifaces[0];
  printf("%s en=%d run=%d wi=%d mac=%s ip=%s nm=%s gw=%s ess=%s\n", i1->GetName().c_str(), i1->IsEnabled(), i1->IsConnected(), i1->IsWireless(), i1->GetMacAddress().c_str(), i1->GetCurrentIPAddress().c_str(), i1->GetCurrentNetmask().c_str(), i1->GetCurrentDefaultGateway().c_str(), i1->GetCurrentWirelessEssId().c_str());
  i1 = ifaces[1];
  printf("%s en=%d run=%d wi=%d mac=%s ip=%s nm=%s gw=%s ess=%s\n", i1->GetName().c_str(), i1->IsEnabled(), i1->IsConnected(), i1->IsWireless(), i1->GetMacAddress().c_str(), i1->GetCurrentIPAddress().c_str(), i1->GetCurrentNetmask().c_str(), i1->GetCurrentDefaultGateway().c_str(), i1->GetCurrentWirelessEssId().c_str());
   
  return 0;
}
*/
