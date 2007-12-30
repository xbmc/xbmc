#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeRecentlyAddedMusicVideos : public CDirectoryNode
    {
    public:
      CDirectoryNodeRecentlyAddedMusicVideos(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
      virtual NODE_TYPE GetChildType();
    };
  }
}

