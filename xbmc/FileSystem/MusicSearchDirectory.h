#pragma once
#include "idirectory.h"

namespace DIRECTORY
{
  class CMusicSearchDirectory : public IDirectory
  {
  public:
    CMusicSearchDirectory(void);
    virtual ~CMusicSearchDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
  };
};
