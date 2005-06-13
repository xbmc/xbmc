#pragma once

#include "idirectory.h"
#include "rarmanager.h"

using namespace DIRECTORY;

namespace DIRECTORY 
{
  class CRarDirectory : public IDirectory
  {
  public:
    CRarDirectory();
    ~CRarDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  };
}
