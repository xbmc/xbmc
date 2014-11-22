//
//  PlexJobs.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#ifndef __Plex_Home_Theater__PlexJobs__
#define __Plex_Home_Theater__PlexJobs__

#include "utils/Job.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "FileItem.h"
#include "guilib/GUIMessage.h"
#include "Client/PlexMediaServerClient.h"
#include "FileSystem/PlexDirectory.h"
#include "threads/CriticalSection.h"
#include "TextureCacheJob.h"
#include "filesystem/File.h"

class CPlexPlayQueue;
typedef boost::shared_ptr<CPlexPlayQueue> CPlexPlayQueuePtr;

////////////////////////////////////////////////////////////////////////////////////////
class CPlexJob : public CJob
{
public:
  CPlexJob() : CJob() {};

  virtual CFileItemListPtr getResult() { return CFileItemListPtr(); }
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexHTTPFetchJob : public CJob
{
  public:
    CPlexHTTPFetchJob(const CURL &url, CPlexServerPtr server = CPlexServerPtr()) : CJob(), m_url(url), m_server(server) {};
  
    bool DoWork();
    void Cancel() { m_http.Cancel(); }
    virtual bool operator==(const CJob* job) const;
  
    XFILE::CCurlFile m_http;
    CStdString m_data;
    CURL m_url;
    CPlexServerPtr m_server;
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexDirectoryFetchJob : public CPlexJob
{
public:
  CPlexDirectoryFetchJob(const CURL &url, CPlexDirectoryCache::CacheStrategies Startegy = CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT) : CPlexJob(), m_url(url) { m_dir.SetCacheStrategy(Startegy);}
  
  virtual bool operator==(const CJob* job) const
  {
    const CPlexDirectoryFetchJob *fjob = static_cast<const CPlexDirectoryFetchJob*>(job);
    if (fjob && fjob->m_url.Get() == m_url.Get())
      return true;
    return false;
  }
  
  virtual const char* GetType() const { return "plexdirectoryfetch"; }
  
  virtual bool DoWork();

  virtual void Cancel()
  {
    m_dir.CancelDirectory();
  }
  
  virtual CFileItemListPtr getResult();

  XFILE::CPlexDirectory m_dir;
  CFileItemList m_items;
  CURL m_url;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexExtraInfoLoaderJob : public CPlexDirectoryFetchJob
{
public:
  CPlexExtraInfoLoaderJob(const CURL &url, const CFileItemListPtr& list, bool block = false)
    : CPlexDirectoryFetchJob(url), m_block(block)
  {
    m_extraList = list;
  }

  bool m_block;
  CFileItemListPtr m_extraList;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexRecursiveFetchJob : public CJob
{
public:
  CPlexRecursiveFetchJob(const CStdString& url, const CStdString& exts, CFileItemList* list)
    : m_url(url), m_exts(exts)
  {
    m_list = list;
  }

  bool DoWork();

  CStdString m_url, m_exts;
  CFileItemList* m_list;
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexSectionFetchJob : public CPlexDirectoryFetchJob
{
public:
  CPlexSectionFetchJob(const CURL& url, int contentType) : CPlexDirectoryFetchJob(url,CPlexDirectoryCache::CACHE_STRATEGY_ALWAYS), m_contentType(contentType) { }
  int m_contentType;
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexMediaServerClientJob : public CJob
{
public:
  CPlexMediaServerClientJob(CURL command, const std::string verb = "GET", const CGUIMessage &msg = CGUIMessage(0, 0, 0, 0), int error = 0) :
    m_url(command), m_verb(verb), m_msg(msg), m_errorMsg(error) {}
  
  virtual bool DoWork();
  
  CURL m_url;
  std::string m_verb;
  CStdString m_data;
  CGUIMessage m_msg;
  CStdString m_postData;
  int m_errorMsg;
  XFILE::CPlexFile m_http;

  virtual void Cancel() { m_http.Cancel(); }

  virtual const char* GetType() const { return "mediaServerClientJob"; }

  /* set this to the shared ptr if we are calling this from the
   * mediaServerClient, otherwise just don't bother */
  CPlexMediaServerClientPtr m_mediaServerClient;

  virtual bool operator==(const CJob* job) const
  {
    CPlexMediaServerClientJob *oJob = (CPlexMediaServerClientJob*)job;
    if (oJob->m_url.Get() == m_url.Get())
      return true;
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexMediaServerClientTimelineJob : public CPlexMediaServerClientJob
{
public:
  CPlexMediaServerClientTimelineJob(const CURL& url, const CFileItemPtr& item)
    : CPlexMediaServerClientJob(url), m_item(item)
  {}

  virtual const char* GetType() const { return "mediaServerClientTimelineJob"; }
  CFileItemPtr m_item;
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexVideoThumbLoaderJob : public CJob
{
public:
  CPlexVideoThumbLoaderJob(CFileItemPtr item) : m_item(item) {}

  virtual bool DoWork();

  virtual bool operator==(const CJob* job) const
  {
    CPlexVideoThumbLoaderJob *ljob = (CPlexVideoThumbLoaderJob*)job;
    return m_item == ljob->m_item;
  }

  CFileItemPtr m_item;
};

////////////////////////////////////////////////////////////////////////////////////////
class CPlexDownloadFileJob : public CJob
{
  public:
    CPlexDownloadFileJob(const CStdString& url, const CStdString& destination) :
      CJob(), m_failed(false), m_url(url), m_destination(destination)
    {};

    virtual void Cancel() { m_http.Cancel(); }

    bool DoWork();
    CStdString m_url;
    CStdString m_destination;
    XFILE::CCurlFile m_http;

    bool m_failed;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexThemeMusicPlayerJob : public CJob
{
  public:
    CPlexThemeMusicPlayerJob(const CStdString& themeUrl) : m_themeUrl(themeUrl) {}
    bool DoWork();
    CStdString m_themeUrl;
    CStdString m_fileToPlay;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexTextureCacheJob : public CTextureCacheJob
{
private:
  XFILE::CFile m_inputFile;
  XFILE::CFile m_outputFile;

public:
  CPlexTextureCacheJob(const CStdString& url, const CStdString& oldHash = "")
    : CTextureCacheJob(url, oldHash)
  {
  }
  virtual bool CacheTexture(CBaseTexture** texture = NULL);
};

#endif /* defined(__Plex_Home_Theater__PlexJobs__) */
