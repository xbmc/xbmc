#pragma once
#include "IFileDirectory.h"

namespace DIRECTORY
{
class CFactoryFileDirectory
{
public:
  CFactoryFileDirectory(void);
  virtual ~CFactoryFileDirectory(void);
  static IFileDirectory* Create(const CStdString& strPath, CFileItem* pItem, const CStdString& strMask="");
};
}
