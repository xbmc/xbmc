#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeAlbumTop100Song : public CDirectoryNode
    {
    public:
      CDirectoryNodeAlbumTop100Song(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
    };
  }
}


