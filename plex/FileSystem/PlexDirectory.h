#pragma once

/*
 * PlexDirectory.h
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <string>

#include "filesystem/CurlFile.h"
#include "FileItem.h"
#include "filesystem/IDirectory.h"
#include "threads/Thread.h"
#include "SortFileItem.h"
#include "PlexTypes.h"
#include "PlexLog.h"

class CURL;
class TiXmlElement;
using namespace std;
using namespace XFILE;

#define PLEX_METADATA_MOVIE   1
#define PLEX_METADATA_SHOW    2
#define PLEX_METADATA_EPISODE 4
#define PLEX_METADATA_TRAILER 5
#define PLEX_METADATA_ARTIST  8
#define PLEX_METADATA_ALBUM   9
#define PLEX_METADATA_TRACK   10
#define PLEX_MEDATATA_PICTURE 11
#define PLEX_METADATA_CLIP    12
#define PLEX_METADATA_PERSON  13 // FIXME, tied to the skin at the moment.
#define METADATA_PHOTO        13
#define METADATA_PHOTO_ALBUM  14
#define PLEX_METADATA_MIXED   100

class CPlexDirectory : public IDirectory, 
                       public CThread
{
 public:
  CPlexDirectory(bool parseResults, bool displayDialog, bool replaceLocalhost, int timeout=300);
  CPlexDirectory(bool parseResults=true, bool displayDialog=true);
  virtual ~CPlexDirectory();
  
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString &strPath) const { return m_dirCacheType; };
  static std::string ProcessUrl(const std::string& parent, const std::string& url, bool isDirectory);
  virtual void SetTimeout(int timeout) { m_timeout = timeout; }
  void SetBody(const CStdString& body) { m_body = body; }
  
  std::string GetData() { return m_data; } 
  
  static std::string ProcessMediaElement(const std::string& parentPath, const char* mediaURL, int maxAge, bool local);
  static std::string BuildImageURL(const std::string& parentURL, const std::string& imageURL, bool local);
  
  static CFileItemListPtr GetFilterList() { return g_filterList; }

  static bool IsHomeVideoSection(const CStdString& url);
  static void AddHomeVideoSection(const CStdString& url);
  
 protected:
  
  virtual void Process();
  
  bool ReallyGetDirectory(const CStdString& strPath, CFileItemList &items);
  void Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, std::string& strFileLabel, std::string& strSecondFileLabel, std::string& strDirLabel, std::string& strSecondDirLabel, bool isLocal);
  void ParseTags(TiXmlElement* element, const CFileItemPtr& item, const std::string& name);
  
  CEvent     m_downloadEvent;
  bool       m_bStop;
  
  CStdString m_url;
  CStdString m_data;
  CStdString m_body;
  bool       m_bSuccess;
  bool       m_bParseResults;
  bool       m_bReplaceLocalhost;
  int        m_timeout;
  CCurlFile  m_http;
  DIR_CACHE_TYPE m_dirCacheType;
  
  static CFileItemListPtr g_filterList;
};

