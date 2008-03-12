
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

CStdString CWIN32Util::MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass, DWORD *dwError)
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

  if(dwError != NULL)
    *dwError = dwRes;

  if(dwRes != NO_ERROR)
  {
    CLog::Log(LOGERROR, "Can't mount %s to %s. Error code %d",strRemote.c_str(), strDrive.c_str(),dwRes);
    return StringUtils::EmptyString;
  }
  
  return strDrive;
}

DWORD CWIN32Util::UmountShare(const CStdString &strPath)
{
  return WNetCancelConnection2((LPCTSTR)strPath.c_str(),NULL,true);
}

CStdString CWIN32Util::MountShare(const CStdString &strPath, DWORD *dwError)
{
  CStdString strURL = strPath;
  CURL url(strURL);
  url.GetURL(strURL);
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  CStdString strPathToShare = "\\\\"+url.GetHostName() + "\\" + url.GetShareName();
  if(!url.GetUserName().IsEmpty())
    return CWIN32Util::MountShare(strPathToShare, strUserName, strPassword, dwError);
  else
    return CWIN32Util::MountShare(strPathToShare, "", "", dwError);
}

CStdString CWIN32Util::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  if(url.GetDomain().length() > 0)
  {
    flat += url.GetDomain();
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += url.GetUserName();
    flat += ":";
    flat += url.GetPassWord();
    flat += "@";
  }
  else if( !url.GetHostName().IsEmpty() && !g_guiSettings.GetString("smb.username").IsEmpty() )
  {
    /* okey this is abit uggly to do this here, as we don't really only url encode */
    /* but it's the simplest place to do so */
    flat += g_guiSettings.GetString("smb.username");
    flat += ":";
    flat += g_guiSettings.GetString("smb.password");
    flat += "@";
  }

  flat += url.GetHostName();  

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += (*it);
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

CStdString CWIN32Util::GetLocalPath(const CStdString &strPath)
{
  CURL url(strPath);
  CStdString strLocalPath = url.GetFileName();
  strLocalPath.Replace(url.GetShareName()+"/","");
  return strLocalPath;
}