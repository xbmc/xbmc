#pragma once

#include "URL.h"

class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static const CStdString GetNextFreeDriveLetter();
  static CStdString MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass, DWORD *dwError=NULL);
  static CStdString MountShare(const CStdString &strPath, DWORD *dwError=NULL);
  static DWORD UmountShare(const CStdString &strPath);
  static CStdString URLEncode(const CURL &url);
  static CStdString CWIN32Util::GetLocalPath(const CStdString &strPath);
  static char CWIN32Util::FirstDriveFromMask (ULONG unitmask);
  static int CWIN32Util::GetDriveStatus(const CStdString &strPath);

private:
  
 
};