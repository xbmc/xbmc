#pragma once
#include "IDirectory.h"
#include "FileSmb.h"

namespace DIRECTORY
{
class CSMBDirectory : public IDirectory
{
public:
  CSMBDirectory(void);
  virtual ~CSMBDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Create(const char* strPath);
  virtual bool Exists(const char* strPath);
  virtual bool Remove(const char* strPath);

  int Open(const CURL &url);

private:
  int OpenDir(const CURL &url, CStdString& strAuth);
};
}
