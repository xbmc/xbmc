#pragma once

#include "IDirectory.h"

namespace DIRECTORY 
{
  class CStackDirectory : public IDirectory
  {
  public:
    CStackDirectory();
    ~CStackDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    static CStdString GetStackedTitlePath(const CStdString &strPath);
    static CStdString GetFirstStackedFile(const CStdString &strPath);
    CStdString ConstructStackPath(const CFileItemList& items, const vector<int> &stack);
  };
}
