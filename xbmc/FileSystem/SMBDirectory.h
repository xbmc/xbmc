#pragma once
#include "idirectory.h"
#include "FileSmb.h"
using namespace XFILE;
using namespace DIRECTORY;
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

  int Open(const CStdString& strPath);

private:
  int OpenDir(CStdString& strAuth);
};
}
