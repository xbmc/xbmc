#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeRecentlyAddedEpisodes : public CDirectoryNode
    {
    public:
      CDirectoryNodeRecentlyAddedEpisodes(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
      virtual NODE_TYPE GetChildType();
    };
  }
}

