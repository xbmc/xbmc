#pragma once
#include "directory.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
  class CISO9660Directory :
    public IDirectory
  {
  public:
    CISO9660Directory(void);
    virtual ~CISO9660Directory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
}