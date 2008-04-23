#pragma once

#include "StdString.h"

#include <vector>

class CDNSNameCache
{
public:
  class CDNSName
  {
  public:
    CStdString m_strHostName;
    CStdString m_strIpAdres;
  };
  CDNSNameCache(void);
  virtual ~CDNSNameCache(void);
  static bool Lookup(const CStdString& strHostName, CStdString& strIpAdres);
  static void Add(const CStdString& strHostName, const CStdString& strIpAdres);

protected:
  static bool GetCached(const CStdString& strHostName, CStdString& strIpAdres);
  static CCriticalSection m_critical;
  std::vector<CDNSName> m_vecDNSNames;
  typedef std::vector<CDNSName>::iterator ivecDNSNames;
};
