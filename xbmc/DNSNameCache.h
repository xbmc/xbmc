#pragma once

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

protected:
  std::vector<CDNSName> m_vecDNSNames;
  typedef std::vector<CDNSName>::iterator ivecDNSNames;
};
