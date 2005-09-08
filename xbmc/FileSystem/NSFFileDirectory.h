#pragma once

#include "ifiledirectory.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
  class CNSFFileDirectory : public IFileDirectory
  {
  public:
    CNSFFileDirectory(void);
    virtual ~CNSFFileDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual bool ContainsFiles(const CStdString& strPath);
  };
};
