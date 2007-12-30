#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeYearSong : public CDirectoryNode
    {
    public:
      CDirectoryNodeYearSong(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& items);
    };
  }
}

