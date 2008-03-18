#pragma once
#include "IDirectory.h"

namespace DIRECTORY
{
class CXBMSDirectory :
      public IDirectory
{
public:
  CXBMSDirectory(void);
  virtual ~CXBMSDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
};
}
