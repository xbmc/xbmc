#pragma once
#include "idirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CXNSDirectory :
    public IDirectory
  {
  public:
    CXNSDirectory(void);
    virtual ~CXNSDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,CFileItemList &items);
  };
}