#pragma once

class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static const CStdString GetNextFreeDriveLetter();
  static CStdString MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass);
  static CStdString MountShare(const CStdString &strPath);

private:
  
 
};