//
//  CocoaUtilsPlus.cpp
//  Plex
//
//  Created by Max Feingold on 10/21/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

#ifndef _WIN32
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include "Settings.h"
#endif

#ifdef _WIN32
#include <WinSock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#define in_addr_t uint32_t
#endif

#ifdef __linux__
#include <sys/utsname.h>
#endif

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "CocoaUtilsPlus.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMachinePlatform()
{
  string platform;
  
#if defined(_WIN32)
  platform = "Windows";
#elif defined(__linux__)
  platform = "Linux";
#else
  platform = "MacOSX";
#endif
  
  return platform;
}

/////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMachinePlatformVersion()
{
  string ver;
  
#if defined(_WIN32)
  
  DWORD dwVersion = GetVersion();
  DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
  DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
  DWORD dwBuildNumber  = (DWORD)(HIWORD(dwVersion));
  
  char str[256];
  sprintf(str, "%d.%d (Build %d)", dwMajorVersion, dwMinorVersion, dwBuildNumber);
  ver = str;
  
#elif defined(__linux__)
  
  struct utsname buf;
  if (uname(&buf) == 0)
  {
    ver = buf.release;
    ver = " (" + string(buf.version) + ")";
  }
  
#else
  
  SInt32 res = 0; 
  Gestalt(gestaltSystemVersionMajor, &res);
  ver = boost::lexical_cast<string>(res) + ".";
  
  Gestalt(gestaltSystemVersionMinor, &res);
  ver += boost::lexical_cast<string>(res) + ".";
  
  Gestalt(gestaltSystemVersionBugFix, &res);
  ver += boost::lexical_cast<string>(res);
  
#endif
  
  return ver;
}

#ifdef __APPLE__
#define SIZE(p) MAX((p).sa_len, sizeof(p))
#endif

vector<in_addr_t> Cocoa_GetLocalAddresses()
{
  vector<in_addr_t> ret;

#ifdef __APPLE__

  static struct ifreq ifreqs[128];
  struct ifconf ifconf;
  memset(&ifconf, 0, sizeof(ifconf));
  memset(&ifreqs, 0, sizeof(ifreqs));
  ifconf.ifc_req = ifreqs;
  ifconf.ifc_len = sizeof(ifreqs);

  int sd = socket(PF_INET, SOCK_STREAM, 0);
  if (ioctl(sd, SIOCGIFCONF, (char *)&ifconf) == 0)
  {
    char* cp = (char *)ifconf.ifc_req;
    char* cplim = cp+ifconf.ifc_len;
    struct ifreq* ifr = ifconf.ifc_req;

    for (; cp<cplim; cp+=(sizeof(ifr->ifr_name) + SIZE(ifr->ifr_addr)))
    {
      ifr = (struct ifreq *)cp;

      struct sockaddr *sa = (struct sockaddr *)&(ifr->ifr_addr);
      if (sa->sa_family == AF_INET || sa->sa_family == AF_LINK)
      {
        struct sockaddr_in* pa = (struct sockaddr_in *)sa;
        ret.push_back(pa->sin_addr.s_addr);
      }
     }
   }

  close(sd);
#endif

#ifdef _WIN32
  char ac[256];
  if (gethostname(ac, sizeof(ac)) != SOCKET_ERROR)
  {
    struct hostent *phe = gethostbyname(ac);
    if (phe)
    {
      for (int i = 0; phe->h_addr_list[i] != 0; ++i) 
      {
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
        ret.push_back(addr.S_un.S_addr);
      }
    }
  }

  // For some reason, 127.0.0.1 isn't included.
  ret.push_back(0x0100007F);

#endif

  return ret;
}

bool Cocoa_IsHostLocal(const string& host)
{
  vector<in_addr_t> localAddresses = Cocoa_GetLocalAddresses();
  bool ret = false;

#ifdef __APPLE__
  hostent* pHost = ::gethostbyname(host.c_str());
  if (pHost)
  {
    BOOST_FOREACH(in_addr_t localAddr, localAddresses)
    {
      for (int x=0; pHost->h_addr_list[x]; x++)
      {
        struct in_addr* in = (struct in_addr *)pHost->h_addr_list[x];
        if (in->s_addr == localAddr)
        {
          ret = true;
          break;
        }
      }
    }
  }
#endif

#ifdef _WIN32
  struct addrinfo hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  struct addrinfo *result = 0;
  DWORD dwRetval = getaddrinfo(host.c_str(), 0, &hints, &result);
  if (dwRetval == 0)
  {
    BOOST_FOREACH(in_addr_t localAddr, localAddresses)
    {
      for (struct addrinfo* ptr=result; ptr != 0; ptr=ptr->ai_next) 
      {
        if (ptr->ai_family == AF_INET)
        {
          struct sockaddr_in* addr = (struct sockaddr_in* )ptr->ai_addr;
          if (addr->sin_addr.s_addr == localAddr)
          {
            ret = true;
            break;
          }
        }
      }
    }
  }
#endif

  CLog::Log(LOGINFO, "Asked to check whether [%s] is localhost => %d", host.c_str(), ret);
  return ret;
}
