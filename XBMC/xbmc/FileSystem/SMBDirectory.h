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

  //MountShare will try to mount the smb share and return the path to the mount point (or empty string if failed)
  static CStdString MountShare(const CStdString &smbPath, const CStdString &strType, const CStdString &strName, 
    const CStdString &strUser, const CStdString &strPass);

  static void UnMountShare(const CStdString &strType, const CStdString &strName);
  static CStdString GetMountPoint(const CStdString &strType, const CStdString &strName);

  static bool MountShare(const CStdString &strType, CShare &share);

private:
  int OpenDir(const CURL &url, CStdString& strAuth);
};
}
