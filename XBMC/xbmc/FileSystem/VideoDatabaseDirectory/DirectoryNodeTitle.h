#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeTitle : public CDirectoryNode
    {
    public:
      CDirectoryNodeTitle(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
    };
  };
};
