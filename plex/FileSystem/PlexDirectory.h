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

#include "PlexTypes.h"
#include "JobManager.h"

#include "utils/log.h"

#include "FileSystem/PlexFile.h"

namespace XFILE
{
  class CPlexDirectory : public IDirectory
  {
  public:
    CPlexDirectory() : m_verb("GET")
    {
    }

    bool GetDirectory(const CURL& url, CFileItemList& items);

    /* plexserver://shared */
    bool GetSharedServerDirectory(CFileItemList& items);

    /* plexserver://channels */
    bool GetChannelDirectory(CFileItemList& items);

    /* plexserver://channeldirectory */
    bool GetOnlineChannelDirectory(CFileItemList& items);

    /* plexserver://playqueue */
    bool GetPlayQueueDirectory(CFileItemList& items);

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

    bool ReadMediaContainer(XML_ELEMENT* root, CFileItemList& mediaContainer);
    void ReadChildren(XML_ELEMENT* element, CFileItemList& container);

  private:
    CStdString m_body;
    CStdString m_data;
    CStdString m_xmlData;
    CURL m_url;

    CPlexFile m_file;

    CStdString m_verb;
  };
}

#endif // PLEXDIRECTORY_H
