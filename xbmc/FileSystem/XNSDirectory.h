#pragma once
#include "directory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CXNSDirectory :
    public CDirectory
  {
  public:
    CXNSDirectory(void);
    virtual ~CXNSDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
}