
#include "stdafx.h"
#include "WIN32Util.h"

#include "../Util.h"

using namespace std;



CWIN32Util::CWIN32Util(void)
{
}

CWIN32Util::~CWIN32Util(void)
{
}


const CStdString CWIN32Util::GetNextFreeDriveLetter()
{
  for(int iDrive='a';iDrive<='z';iDrive++)
  {
    CStdString strDrive;
    strDrive.Format("%c:",iDrive);
    int iType = GetDriveType(strDrive);
    if(iType == DRIVE_NO_ROOT_DIR && iDrive != 'a' && iDrive != 'b' && iDrive != 'q' && iDrive != 'p' && iDrive != 't' && iDrive != 'z')
      return strDrive;
  }
  return StringUtils::EmptyString;
}

CStdString CWIN32Util::MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass)
{
  NETRESOURCE nr;
  memset(&nr,0,sizeof(nr));
  CStdString strRemote = smbPath;
  CStdString strDrive = CWIN32Util::GetNextFreeDriveLetter();

  if(strDrive == StringUtils::EmptyString)
    return StringUtils::EmptyString;

  strRemote.Replace('/', '\\');

  nr.lpRemoteName = (LPTSTR)(LPCTSTR)strRemote.c_str();
  nr.lpLocalName  = (LPTSTR)(LPCTSTR)strDrive.c_str();
  nr.dwType       = RESOURCETYPE_DISK;

  DWORD dwRes = WNetAddConnection2(&nr,(LPCTSTR)strUser.c_str(), (LPCTSTR)strPass.c_str(), NULL);

  if(dwRes != NO_ERROR)
  {
    CLog::Log(LOGERROR, "Can't mount %s to %s. Error code %d",strRemote.c_str(), strDrive.c_str(),dwRes);
    return StringUtils::EmptyString;
  }
  
  return strDrive;
}

CStdString CWIN32Util::MountShare(const CStdString &strPath)
{
  CStdString strURL = strPath;
  CURL url(strURL);
  url.GetURL(strURL);
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  CStdString strPathToShare = "\\\\"+url.GetHostName() + "\\" + url.GetShareName();
  if(!url.GetUserName().IsEmpty())
    return CWIN32Util::MountShare(strPathToShare, strUserName, strPassword);
  else
    return CWIN32Util::MountShare(strPathToShare, "", "");
}