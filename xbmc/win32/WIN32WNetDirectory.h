#pragma once

#include "FileSystem/IDirectory.h"
#include "WIN32FileWNet.h"

namespace DIRECTORY
{

class CWNetDirectory : public IDirectory
{
public:
  CWNetDirectory(void);
  virtual ~CWNetDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Create(const char* strPath);
  virtual bool Exists(const char* strPath);
  virtual bool Remove(const char* strPath);

private:
  CStdString strMntPoint;
};
}
