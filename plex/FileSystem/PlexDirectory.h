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
#include "XBMCTinyXML.h"
#include "FileItem.h"

#include "PlexAttributeParser.h"

#include <map>

#include "PlexTypes.h"
#include "JobManager.h"

namespace XFILE
{
  class CPlexDirectory : public IDirectory, public CJobQueue
  {
    public:

      class CPlexDirectoryFetchJob : public CJob
      {
        public:
          CPlexDirectoryFetchJob(const CURL &url) : CJob(), m_url(url) {}

          virtual bool operator==(const CJob* job) const
          {
            const CPlexDirectoryFetchJob *fjob = static_cast<const CPlexDirectoryFetchJob*>(job);
            if (fjob && fjob->m_url.Get() == m_url.Get())
              return true;
            return false;
          }

          virtual const char* GetType() const { return "plexdirectoryfetch"; }

          virtual bool DoWork();

          CFileItemListPtr m_items;
          CURL m_url;
      };

      CPlexDirectory(CURL augmentedUrl = CURL(), bool usePaging = false) : m_augmentedURL(augmentedUrl), m_usePaging(usePaging) {}

      virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
      virtual void CancelDirectory();

      static EPlexDirectoryType GetDirectoryType(const CStdString& typeStr);
      static CStdString GetDirectoryTypeString(EPlexDirectoryType typeNr);

      static CStdString GetContentFromType(EPlexDirectoryType typeNr);

      /* Legacy functions we need to revisit */
      void SetBody(const CStdString& body) { m_body = body; }
      static CFileItemListPtr GetFilterList() { return CFileItemListPtr(); }
      CStdString GetData() const { return m_data; }

      static void CopyAttributes(TiXmlElement* element, CFileItem& fileItem, const CURL &url);
      static CFileItemPtr NewPlexElement(TiXmlElement *element, CFileItem &parentItem, const CURL &url = CURL());

      virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    private:
      bool ReadMediaContainer(TiXmlElement* root, CFileItemList& mediaContainer);
      void ReadChildren(TiXmlElement* element, CFileItemList& container);

      void DoAugmentation(CFileItemList& fileItems);

      CURL m_url;
      CURL m_augmentedURL;
      CFileItemListPtr m_augmentedItem;

      CEvent m_augmentationEvent;

      bool m_usePaging;

      CStdString m_body;
      CStdString m_data;
  };
}



#endif // PLEXDIRECTORY_H
