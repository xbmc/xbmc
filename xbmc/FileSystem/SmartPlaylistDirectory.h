#pragma once

#include "ifiledirectory.h"

using namespace DIRECTORY;

namespace DIRECTORY 
{
  class CSmartPlaylistDirectory : public IFileDirectory
  {
  public:
    CSmartPlaylistDirectory();
    ~CSmartPlaylistDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);

    static CStdString GetPlaylistByName(const CStdString& name);
  };
}
