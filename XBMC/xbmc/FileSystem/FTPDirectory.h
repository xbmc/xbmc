#pragma once
#include "idirectory.h"
#include "FTPUtil.h"
using namespace DIRECTORY;

namespace DIRECTORY
{
  class CFTPDirectory : public IDirectory
  {
    public:
      CFTPDirectory(void);
      virtual ~CFTPDirectory(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    private:
      CFTPUtil FTPUtil;
  };
}