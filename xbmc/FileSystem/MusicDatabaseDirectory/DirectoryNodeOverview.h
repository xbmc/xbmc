#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeOverview : public CDirectoryNode
    {
    public:
      CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  }
}

