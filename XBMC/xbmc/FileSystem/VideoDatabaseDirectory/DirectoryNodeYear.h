#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeYear : public CDirectoryNode
    {
    public:
      CDirectoryNodeYear(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
      virtual NODE_TYPE GetChildType();
    };
  }
}

