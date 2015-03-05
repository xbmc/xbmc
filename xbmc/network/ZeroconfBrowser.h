#pragma once

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

#include <string>
#include <set>
#include <vector>
#include <map>

#ifdef TARGET_WINDOWS
#undef SetPort // WIN32INCLUDES this is defined as SetPortA in WinSpool.h which is being included _somewhere_
#endif

//forwards
class CCriticalSection;

/// this class provides support for zeroconf browsing
class CZeroconfBrowser
{
public:
  class ZeroconfService
  {
    public:
      typedef std::map<std::string, std::string> tTxtRecordMap;

      ZeroconfService();
      ZeroconfService(const std::string& fcr_name, const std::string& fcr_type, const std::string& fcr_domain);

      /// easy conversion to string and back (used in czeronfdiretory to store this service)
      ///@{
      static std::string toPath(const ZeroconfService& fcr_service);
      static ZeroconfService fromPath(const std::string& fcr_path); //throws std::runtime_error on failure
      ///@}

      /// general access methods
      ///@{
      void SetName(const std::string& fcr_name);
      const std::string& GetName() const {return m_name;}

      void SetType(const std::string& fcr_type);
      const std::string& GetType() const {return m_type;}

      void SetDomain(const std::string& fcr_domain);
      const std::string& GetDomain() const {return m_domain;}
      ///@}

      /// access methods needed during resolve
      ///@{
      void SetIP(const std::string& fcr_ip);
      const std::string& GetIP() const {return m_ip;}

      void SetHostname(const std::string& fcr_hostname);
      const std::string& GetHostname() const {return m_hostname;}

      void SetPort(int f_port);
      int GetPort() const {return m_port;}
    
      void SetTxtRecords(const tTxtRecordMap& txt_records);
      const tTxtRecordMap& GetTxtRecords() const { return m_txtrecords_map;}
      ///@}
    private:
      //3 entries below identify a service
      std::string m_name;
      std::string m_type;
      std::string m_domain;

      //2 entries below store 1 ip:port pair for this service
      std::string m_ip;
      int        m_port;

      //used for mdns in case dns resolution fails
      //we store the hostname and resolve with mdns functions again
      std::string m_hostname;
      
      //1 entry below stores the txt-record as a key value map for this service
      tTxtRecordMap m_txtrecords_map;    
  };

  // starts browsing
  void Start();

  // stops browsing
  void Stop();

  ///returns the list of found services
  /// if this is updated, the following message with "zeroconf://" as path is sent:
  /// CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
  std::vector<ZeroconfService> GetFoundServices();
  ///@}

  // resolves a ZeroconfService to ip + port
  // @param fcr_service the service to resolve
  // @param f_timeout timeout in seconds for resolving
  //   the protocol part of CURL is the raw zeroconf service type
  //   added with AddServiceType (== needs further processing! e.g. _smb._tcp -> smb)
  // @return true if it was successfully resolved (or scheduled), false if resolve
  //         failed (async or not)
  bool ResolveService(ZeroconfService& fr_service, double f_timeout = 1.0);

  // class methods
  // access to singleton; singleton gets created on call if not existent
  // if zeroconf is disabled (!HAS_ZEROCONF), this will return a dummy implementation that
  // just does nothings, otherwise the platform specific one
  static CZeroconfBrowser* GetInstance();
  // release the singleton; (save to call multiple times)
  static void ReleaseInstance();
  // returns false if ReleaseInstance() was called befores
  static bool IsInstantiated() { return  smp_instance != 0; }

  virtual void ProcessResults() {}

  /// methods for browsing and getting results of it
  ///@{
  /// adds a service type for browsing
  /// @param fcr_service_type the service type as string, e.g. _smb._tcp.
  /// @return false if it was already there
  bool AddServiceType(const std::string& fcr_service_type);

  /// remove the specified service from discovery
  /// @param fcr_service_type the service type as string, e.g. _smb._tcp.
  /// @return if it was not found
  bool RemoveServiceType(const std::string& fcr_service_type);

protected:
  //singleton: we don't want to get instantiated nor copied or deleted from outside
  CZeroconfBrowser();
  CZeroconfBrowser(const CZeroconfBrowser&);
  virtual ~CZeroconfBrowser();

  // pure virtual methods to implement for OS specific implementations
  virtual bool doAddServiceType(const std::string& fcr_service_type) = 0;
  virtual bool doRemoveServiceType(const std::string& fcr_service_type) = 0;
  virtual std::vector<ZeroconfService> doGetFoundServices() = 0;
  virtual bool doResolveService(ZeroconfService& fr_service, double f_timeout) = 0;

private:
  struct ServiceInfo
  {
    std::string type;
  };

  //protects data
  CCriticalSection* mp_crit_sec;
  typedef std::set<std::string> tServices;
  tServices m_services;
  bool m_started;

  //protects singleton creation/destruction
  static long sm_singleton_guard;
  static CZeroconfBrowser* smp_instance;
};
#include <iostream>

//debugging helper
inline std::ostream& operator<<(std::ostream& o, const CZeroconfBrowser::ZeroconfService& service){
  o << "(" << service.GetName() << "|" << service.GetType() << "|" << service.GetDomain() << ")";
  return o;
}

//inline methods
inline bool operator<(CZeroconfBrowser::ZeroconfService const& fcr_lhs, CZeroconfBrowser::ZeroconfService const& fcr_rhs)
{
  return (fcr_lhs.GetName() +  fcr_lhs.GetType() + fcr_lhs.GetDomain() < fcr_rhs.GetName() + fcr_rhs.GetType() + fcr_rhs.GetDomain());
}

inline bool operator==(CZeroconfBrowser::ZeroconfService const& fcr_lhs, CZeroconfBrowser::ZeroconfService const& fcr_rhs)
{
  return (fcr_lhs.GetName() == fcr_rhs.GetName() && fcr_lhs.GetType() == fcr_rhs.GetType() && fcr_lhs.GetDomain() == fcr_rhs.GetDomain() );
}
