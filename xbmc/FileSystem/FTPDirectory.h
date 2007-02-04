#pragma once
#include "idirectory.h"

namespace DIRECTORY
{
  class CFTPDirectory : public IDirectory
  {
    public:
      CFTPDirectory(void);
      virtual ~CFTPDirectory(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    private:      
  };
}