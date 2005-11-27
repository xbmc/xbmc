#pragma once

#include "idirectory.h"

using namespace DIRECTORY;

namespace DIRECTORY 
{
  class CStackDirectory : public IDirectory
  {
  public:
    CStackDirectory();
    ~CStackDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    CStdString GetStackedTitlePath(const CStdString &strPath);
  };
}
