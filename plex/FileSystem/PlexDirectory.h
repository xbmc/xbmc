//
//  PlexDirectory.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-05.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXDIRECTORY_H
#define PLEXDIRECTORY_H

#include "filesystem/IDirectory.h"
#include "URL.h"
#include "XMLChoice.h"
#include "FileItem.h"

#include "PlexAttributeParser.h"

#include <map>
#include <boost/foreach.hpp>
#include <boost/scoped_array.hpp>

#include "PlexTypes.h"
#include "JobManager.h"

#include "utils/log.h"

#include "FileSystem/PlexFile.h"
#include "FileSystem/PlexDirectoryCache.h"

namespace XFILE
{
  class CPlexDirectory : public IDirectory
  {
  public:
    CPlexDirectory()
      : m_verb("GET")
      , m_xmlData(new char[1024])
      , m_cacheStrategy(CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT)
      , m_showErrors(false)
    {
    }

    // make it easy to override network access in tests.
    virtual bool GetXMLData(CStdString& data);
    bool GetDirectory(const CURL& url, CFileItemList& items);

    /* plexserver://shared */
    bool GetSharedServerDirectory(CFileItemList& items);

    /* plexserver://channels */
    bool GetChannelDirectory(CFileItemList& items);

    /* plexserver://channeldirectory */
    bool GetOnlineChannelDirectory(CFileItemList& items);

    /* plexserver://playqueue */
    bool GetPlayQueueDirectory(ePlexMediaType type, CFileItemList& items);
    
    /* plexserver://playlists */
    bool GetPlaylistsDirectory(CFileItemList& items, CStdString options);

    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items)
    {
      return GetDirectory(CURL(strPath), items);
    }

    virtual void CancelDirectory();

    static EPlexDirectoryType GetDirectoryType(const CStdString& typeStr);
    static CStdString GetDirectoryTypeString(EPlexDirectoryType typeNr);

    static CStdString GetContentFromType(EPlexDirectoryType typeNr);

    void SetHTTPVerb(const CStdString& verb)
    {
      m_verb = verb;
    }
    
    inline CStdString getHTTPVerb() { return m_verb; };

    /* Legacy functions we need to revisit */
    void SetBody(const CStdString& body)
    {
      m_body = body;
    }

    static CFileItemListPtr GetFilterList()
    {
      return CFileItemListPtr();
    }

    CStdString GetData() const
    {
      return m_data;
    }

    static void CopyAttributes(XML_ELEMENT* element, CFileItem* fileItem, const CURL& url);
    static CFileItemPtr NewPlexElement(XML_ELEMENT* element, const CFileItem& parentItem,
                                       const CURL& url = CURL());

    static bool IsFolder(const CFileItemPtr& item, XML_ELEMENT* element);

    long GetHTTPResponseCode() const
    {
      return m_file.GetLastHTTPResponseCode();
    }

    virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;

    virtual bool IsAllowed(const CStdString& strFile) const
    {
      return true;
    }

    static bool CachePath(const CStdString& path);

    inline void SetCacheStrategy(CPlexDirectoryCache::CacheStrategies Strategy) { m_cacheStrategy = Strategy; }

    bool ReadMediaContainer(XML_ELEMENT* root, CFileItemList& mediaContainer);
    void ReadChildren(XML_ELEMENT* element, CFileItemList& container);

    inline void SetShowErrors(bool showErrors)  { m_showErrors = showErrors; }
    inline bool ShouldShowErrors()  { return m_showErrors; }

  private:
    CStdString m_body;
    CStdString m_data;
    boost::scoped_array<char> m_xmlData;
    CURL m_url;
    CPlexDirectoryCache::CacheStrategies m_cacheStrategy;

    CPlexFile m_file;

    CStdString m_verb;
    bool m_showErrors;
  };
}

#endif // PLEXDIRECTORY_H
