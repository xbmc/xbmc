#pragma once

#include "directory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{

  class CHDDirectory :
    public CDirectory
  {
  public:
    CHDDirectory(void);
    virtual ~CHDDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
};
