#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeMovies : public CDirectoryNode
    {
    public:
      CDirectoryNodeMovies(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  };
};

