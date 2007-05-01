#pragma once

#include "IFileDirectory.h"
#include "../cores/paplayer/DllSidplay2.h"

namespace DIRECTORY
{
  class CSIDFileDirectory : public IFileDirectory
  {
  public:
    CSIDFileDirectory(void);
    virtual ~CSIDFileDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual bool ContainsFiles(const CStdString& strPath);
  private:
    DllSidplay2 m_dll;
  };
};
