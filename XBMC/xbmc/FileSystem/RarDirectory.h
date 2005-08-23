#pragma once

#include "ifiledirectory.h"
#include "rarmanager.h"

using namespace DIRECTORY;

namespace DIRECTORY 
{
  class CRarDirectory : public IFileDirectory
  {
  public:
    CRarDirectory();
    ~CRarDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool ContainsFiles(const CStdString& strPath);
  };
}
