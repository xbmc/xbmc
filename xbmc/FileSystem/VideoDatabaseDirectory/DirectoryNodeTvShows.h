#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeTvShows : public CDirectoryNode
    {
    public:
      CDirectoryNodeTvShows(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  };
};

