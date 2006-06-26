#pragma once

#include "idirectory.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
  class CPlaylistDirectory : public IDirectory
  {
  public:
    CPlaylistDirectory(void);
    virtual ~CPlaylistDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  };
};
