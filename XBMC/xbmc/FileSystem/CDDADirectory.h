#pragma once

#include "directory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{

  class CCDDADirectory :
    public CDirectory
  {
  public:
    CCDDADirectory(void);
    virtual ~CCDDADirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
};
