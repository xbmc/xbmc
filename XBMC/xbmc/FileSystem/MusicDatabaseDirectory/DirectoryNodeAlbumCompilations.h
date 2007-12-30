#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeAlbumCompilations : public CDirectoryNode
    {
    public:
      CDirectoryNodeAlbumCompilations(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  }
}


