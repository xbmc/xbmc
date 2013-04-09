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

namespace XFILE
{
  class CPlexDirectory : public IDirectory
  {
    public:

      CPlexDirectory();

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

    private:
      bool ReadMediaContainer(TiXmlElement* root, CFileItemList& mediaContainer);
      void ReadChildren(TiXmlElement* element, CFileItemList& container);

      CURL m_url;

      CStdString m_body;
      CStdString m_data;
  };
}



#endif // PLEXDIRECTORY_H
