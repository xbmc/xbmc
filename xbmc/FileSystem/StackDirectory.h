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
    CStdString ConstructStackPath(const CFileItemList& items, const vector<int> &stack);
  };
}
