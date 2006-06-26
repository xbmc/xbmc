#pragma once

#include "ifiledirectory.h"

using namespace DIRECTORY;

namespace DIRECTORY 
{
  class CPlaylistFileDirectory : public IFileDirectory
  {
  public:
    CPlaylistFileDirectory();
    ~CPlaylistFileDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Remove(const char *strPath) { return CFile::Delete(strPath); };
  };
}
