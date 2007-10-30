#pragma once

#include "IFileDirectory.h"
#include "RarManager.h"

namespace DIRECTORY 
{
  class CRarDirectory : public IFileDirectory
  {
  public:
    CRarDirectory();
    ~CRarDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Exists(const char* strPath);
  };
}
