#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeTitleMovies : public CDirectoryNode
    {
    public:
      CDirectoryNodeTitleMovies(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
    };
  }
}
