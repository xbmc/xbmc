#pragma once

#include "IDirectory.h"
#include "MemoryUnits/IFileSystem.h"

namespace DIRECTORY
{
  class CMemUnitDirectory : public IDirectory
  {
  public:
    CMemUnitDirectory(void);
    virtual ~CMemUnitDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Create(const char* strPath);
    virtual bool Exists(const char* strPath);
    virtual bool Remove(const char* strPath);
  protected:
    IFileSystem *GetFileSystem(const CStdString &path);
  };
};
