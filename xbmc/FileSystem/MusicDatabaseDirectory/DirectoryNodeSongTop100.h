#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeSongTop100 : public CDirectoryNode
    {
    public:
      CDirectoryNodeSongTop100(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
    };
  }
}

