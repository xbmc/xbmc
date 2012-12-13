//
//  PlexAutoUpdate.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATE_H
#define PLEXAUTOUPDATE_H

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

class CAutoUpdateInfoVersion
{
  public:
    CAutoUpdateInfoVersion() {};
    CAutoUpdateInfoVersion(const std::string& versionStr) : m_versionStr(versionStr)
    {
      std::vector<std::string> split;
      boost::split(split, versionStr, boost::is_any_of("."));
      BOOST_FOREACH(std::string v, split)
      {
        versionSplit.push_back(boost::lexical_cast<int>(v));
      }
    }

    bool operator==(CAutoUpdateInfoVersion& v2)
    {
      if (NumVersionElements() != v2.NumVersionElements())
        return false;

      for(int i = 0; i < v2.NumVersionElements(); i++)
      {
        if (GetVersionElement(i) != v2.GetVersionElement(i))
          return false;
      }

      return true;
    }

    bool operator<(CAutoUpdateInfoVersion& v2)
    {
      for(int i = 0; i < v2.NumVersionElements(); i++)
      {
        if (GetVersionElement(i) > v2.GetVersionElement(i))
          return false;

        if (GetVersionElement(i) < v2.GetVersionElement(i))
          return true;
      }

      return false;
    }

    bool operator>(CAutoUpdateInfoVersion& v2)
    {
      return !operator<(v2);
    }

    bool operator!=(CAutoUpdateInfoVersion& v2)
    {
      return !operator==(v2);
    }

    int GetVersionElement(int i) const
    {
      return versionSplit[i];
    }

    int NumVersionElements() const
    {
      return versionSplit.size();
    }

    std::string GetVersionString() const
    {
      return m_versionStr;
    }

    CAutoUpdateInfoVersion& operator=(const CAutoUpdateInfoVersion& version)
    {
      versionSplit = version.versionSplit;
      m_versionStr = version.m_versionStr;

      return *this;
    }

  private:
    std::vector<int> versionSplit;
    std::string m_versionStr;

};

class CAutoUpdateInfo
{
  public:
    CAutoUpdateInfo(const std::string& title, const std::string& relnoteUrl, const std::string& date) :
      m_title(title),
      m_relnoteUrl(relnoteUrl),
      m_date(date)
    {};

    CAutoUpdateInfo() {};

    CAutoUpdateInfo& operator=(const CAutoUpdateInfo& info)
    {
      m_title = info.m_title;
      m_relnoteUrl = info.m_relnoteUrl;
      m_date = info.m_date;
      m_enclosureSize = info.m_enclosureSize;
      m_enclosureUrl = info.m_enclosureUrl;
      m_enclosureVersion = info.m_enclosureVersion;

      return *this;
    }

    std::string m_title;
    std::string m_relnoteUrl;
    std::string m_date;

    /* enclosure */
    std::string m_enclosureUrl;
    CAutoUpdateInfoVersion m_enclosureVersion;
    int64_t m_enclosureSize;
};

class CAutoUpdateInstallerBase
{
  public:
    CAutoUpdateInstallerBase(const std::string& resourcePath) : m_resourcePath(resourcePath) {};
    virtual bool InstallUpdate(const std::string& file, std::string& unpackPath) { return false; }
  
  protected:
    std::string m_resourcePath;
};

typedef std::vector<CAutoUpdateInfo> CAutoUpdateInfoList;

class CPlexAutoUpdate;

class CAutoUpdateFunctionsBase
{
  public:

    CAutoUpdateFunctionsBase(CPlexAutoUpdate* updater)
    {
      m_updater = updater;
    }

    virtual bool FetchUrlData(const std::string& url, std::string& data) { return false; }
    virtual bool DownloadFile(const std::string& url, std::string& localPath) { return false; }
    virtual bool ParseXMLData(const std::string& xmlData, CAutoUpdateInfoList& infoList) { return false; }
    virtual void LogDebug(const std::string& msg) {};
    virtual void LogInfo(const std::string& msg) {};
    virtual bool ShouldWeInstall(const std::string& localPath) { return false; }
  
    virtual std::string GetResourcePath() const { return std::string(); }

    /* Need to call CPlexAutoUpdate->DownloadNewVersion() when user confirms */
    virtual void NotifyNewVersion() {};

    CPlexAutoUpdate* m_updater;
};


#ifdef __PLEX__XBMC__
#include "plex/AutoUpdate/PlexAutoUpdateFunctionsXBMC.h"
#endif

class CPlexAutoUpdate
{
  public:
    CPlexAutoUpdate(const std::string& updateUrl, int searchFrequency = 21600);
    void Stop();
    bool CheckForNewVersion();
    bool DownloadNewVersion();

    CAutoUpdateInfo GetNewVersion() const
    {
      return m_newVersion;
    }

    CAutoUpdateInfoVersion GetCurrentVersion() const
    {
      return m_currentVersion;
    }

  private:
    boost::mutex m_lock;
    boost::condition_variable m_waitSleepCond;

    CAutoUpdateFunctionsBase *m_functions;
    CAutoUpdateInstallerBase *m_installer;

    void run();
    std::string m_updateUrl;
    int m_searchFrequency;

    bool m_stop;

    CAutoUpdateInfo m_newVersion;
    CAutoUpdateInfoVersion m_currentVersion;
};

#endif // PLEXAUTOUPDATE_H
