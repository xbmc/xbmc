#pragma once

#include "IFileDirectory.h"

namespace DIRECTORY 
{
  class CPlaylistFileDirectory : public IFileDirectory
  {
  public:
    CPlaylistFileDirectory();
    ~CPlaylistFileDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Remove(const char *strPath) { return XFILE::CFile::Delete(strPath); };
  };
}
