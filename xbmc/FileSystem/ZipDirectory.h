#pragma once

#include "IFileDirectory.h"
#include "ZipManager.h"

namespace DIRECTORY 
{
  class CZipDirectory : public IFileDirectory
  {
  public:
    CZipDirectory();
    ~CZipDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
  };
}
