#pragma once

#include "idirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{

  class CCDDADirectory :
    public IDirectory
  {
  public:
    CCDDADirectory(void);
    virtual ~CCDDADirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
};
