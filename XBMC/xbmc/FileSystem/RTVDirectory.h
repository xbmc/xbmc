// RTVDirectory.h: interface for the CRTVDirectory class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "directory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CRTVDirectory :
    public CDirectory
  {
  public:
    CRTVDirectory(void);
    virtual ~CRTVDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
}