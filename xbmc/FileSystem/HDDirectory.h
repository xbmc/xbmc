#pragma once

#include "idirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{

  class CHDDirectory : public IDirectory
  {
  public:
    CHDDirectory(void);
    virtual ~CHDDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath,CFileItemList &items);
    virtual bool Create(const char* strPath);
    virtual bool Exists(const char* strPath);
    virtual bool Remove(const char* strPath);
  };
};
