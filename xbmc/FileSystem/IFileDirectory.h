#pragma once
#include "idirectory.h"
using namespace DIRECTORY;
namespace DIRECTORY
{
class IFileDirectory : public IDirectory
{
public:
  virtual ~IFileDirectory(void) {};
  virtual bool ContainsFiles(const CStdString& strPath)=0;
};

}
