#pragma once

#include "idirectory.h"
#include "ZipManager.h"

namespace DIRECTORY 
{
  class CZipDirectory : public IDirectory
  {
  public:
    CZipDirectory();
    ~CZipDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  };
}
