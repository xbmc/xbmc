#pragma once

#include "IFileDirectory.h"

namespace DIRECTORY 
{
  class CSmartPlaylistDirectory : public IFileDirectory
  {
  public:
    CSmartPlaylistDirectory();
    ~CSmartPlaylistDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Remove(const char *strPath) { return XFILE::CFile::Delete(strPath); };

    static CStdString GetPlaylistByName(const CStdString& name);
  };
}
