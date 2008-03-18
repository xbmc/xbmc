// RTVDirectory.h: interface for the CRTVDirectory class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "IDirectory.h"

namespace DIRECTORY
{
class CRTVDirectory :
      public IDirectory
{
public:
  CRTVDirectory(void);
  virtual ~CRTVDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};
}
