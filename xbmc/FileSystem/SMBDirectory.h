#pragma once
#include "directory.h"
#include "FileSmb.h"
using namespace XFILE;
using namespace DIRECTORY;
namespace DIRECTORY
{
  class CSMBDirectory : public CDirectory
  {
  public:
    CSMBDirectory(void);
    virtual ~CSMBDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
	private:
  };
}