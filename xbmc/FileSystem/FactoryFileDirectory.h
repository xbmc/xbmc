#pragma once
#include "ifiledirectory.h"
using namespace DIRECTORY;
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
