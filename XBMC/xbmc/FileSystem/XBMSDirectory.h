#pragma once
#include "idirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CXBMSDirectory :
    public IDirectory
  {
  public:
    CXBMSDirectory(void);
    virtual ~CXBMSDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,CFileItemList &items);
  };
}