#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeAlbumRecentlyAdded : public CDirectoryNode
    {
    public:
      CDirectoryNodeAlbumRecentlyAdded(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  }
}

