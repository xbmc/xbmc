#pragma once

#include "IFileDirectory.h"

namespace DIRECTORY
{
  class COGGFileDirectory : public IFileDirectory
  {
  public:
    COGGFileDirectory(void);
    virtual ~COGGFileDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual bool ContainsFiles(const CStdString& strPath);
  };
};
