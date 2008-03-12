
#include "stdafx.h"
#include "WIN32WNetDirectory.h"
#include "../Util.h"
#include "../xbox/IoSupport.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/iso9660.h"
#include "../GUIPassword.h"
#include "WIN32Util.h"
#include "Application.h"
#include "../GUIPassword.h"

using namespace AUTOPTR;
using namespace DIRECTORY;

CWNetDirectory::CWNetDirectory(void)
{
  strMntPoint = "";
}

CWNetDirectory::~CWNetDirectory(void)
{
  CWIN32Util::UmountShare(strMntPoint.c_str());
  strMntPoint = "";
}

bool CWNetDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{

  WIN32_FIND_DATA wfd;

  CStdString strPath=strPath1;
  CStdString strAuth;
  CURL url(strPath);

  if(strMntPoint==StringUtils::EmptyString)
    strMntPoint = OpenDir(url,strAuth);

  if(strMntPoint==StringUtils::EmptyString)
    return false;

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strMntPoint;

  memset(&wfd, 0, sizeof(wfd));
  CUtil::AddFileToFolder(strRoot,CWIN32Util::GetLocalPath(strPath1),strRoot);

  strRoot.Replace("/", "\\");

  CStdString strSearchMask = strRoot;

  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  
  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid()) 
      return Exists(strPath1);

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          CStdString strDir = wfd.cFileName;
          if (strDir != "." && strDir != "..")
          {
            CStdString strLabel=wfd.cFileName;

            g_charsetConverter.stringCharsetToUtf8(strLabel);

            CFileItem *pItem = new CFileItem(strLabel);
            pItem->m_strPath = strPath1;
            pItem->m_strPath += wfd.cFileName;
            g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);
            pItem->m_bIsFolder = true;
            CUtil::AddSlashAtEnd(pItem->m_strPath);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            vecCacheItems.Add(pItem);

            items.Add(new CFileItem(*pItem));
          }
        }
        else
        {
          CStdString strLabel=wfd.cFileName;

          g_charsetConverter.stringCharsetToUtf8(strLabel);

          CFileItem *pItem = new CFileItem(strLabel);
          pItem->m_strPath = strPath1;
          pItem->m_strPath += wfd.cFileName;

          g_charsetConverter.stringCharsetToUtf8(pItem->m_strPath);

          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          if ( IsAllowed( wfd.cFileName) )

          {
            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));
          }
          else
            vecCacheItems.Add(pItem);
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

CStdString CWNetDirectory::Open(const CURL &url)
{
  CStdString strAuth;  
  return OpenDir(url, strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
CStdString CWNetDirectory::OpenDir(const CURL& url, CStdString& strAuth)
{
  DWORD dwError=0;

  
  /* make a writeable copy */
  CURL urlIn(url);

  /* set original url */
  strAuth = CWIN32Util::URLEncode(urlIn);

  CStdString strPath;
  CStdString strShare;
  /* must url encode this as, auth code will look for the encoded value */
  strShare  = urlIn.GetHostName();
  strShare += "/";
  strShare += urlIn.GetShareName();

  IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);
  if(it != g_passwordManager.m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURL url(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();
    CStdString strUserName = url.GetUserName();
    urlIn.SetPassword(strPassword);
    urlIn.SetUserName(strUserName);
  }
  
  // for a finite number of attempts use the following instead of the while loop:
  // for(int i = 0; i < 3, fd < 0; i++)
  while (strMntPoint == StringUtils::EmptyString)
  {    
    /* samba has a stricter url encoding, than our own.. CURL can decode it properly */
    /* however doesn't always encode it correctly (spaces for example) */
    strPath = CWIN32Util::URLEncode(urlIn);
  
    
    CLog::Log(LOGDEBUG, "%s - Using authentication url %s", __FUNCTION__, strPath.c_str());
    {     
      strMntPoint = CWIN32Util::MountShare(strPath, &dwError);
    }
    
    if (strMntPoint == StringUtils::EmptyString)
    {

      // if we have an 'invalid handle' error we don't display the error
      // because most of the time this means there is no cdrom in the server's
      // cdrom drive.
      if (dwError == ERROR_NO_NET_OR_BAD_PATH)
        break;

      // NOTE: be sure to warn in XML file about Windows account lock outs when too many attempts
      // if the error is access denied, prompt for a valid user name and password
      if (dwError == ERROR_ACCESS_DENIED)
      {
        if (m_allowPrompting)
        {
          g_passwordManager.SetSMBShare(strPath);
          if (!g_passwordManager.GetSMBShareUserPassword())  // Do this bit via a threadmessage?
          	break;

          /* must do this as our urlencoding for spaces is invalid for samba */
          /* and doing double url encoding will fail */
          /* curl doesn't decode / encode filename yet */
          CURL urlnew( g_passwordManager.GetSMBShare() );
          urlIn.SetUserName(urlnew.GetUserName());
          urlIn.SetPassword(urlnew.GetPassWord());
        }
        else
          break;
      }
      else
      {
        CStdString cError;
        if (dwError == ERROR_BAD_NET_NAME)
          cError.Format(g_localizeStrings.Get(770).c_str(),dwError);
        else
          cError.Format("Error: %x",dwError);
        
        if (m_allowPrompting)
        {
          CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialog->SetHeading(257);
          pDialog->SetLine(0, cError);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, "");

          ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
          g_application.getApplicationMessenger().SendMessage(tMsg, false);
        }
        break;
      }
    }
  }

  if (strMntPoint == StringUtils::EmptyString)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\n nt_err : '%x' ", strPath.c_str(),dwError );
  }
  else if (strPath != strAuth && !strShare.IsEmpty()) // we succeeded so, if path was changed, return the correct one and cache it
  {
    g_passwordManager.m_mapSMBPasswordCache[strShare] = strPath;
    strAuth = strPath;
  }  

  return strMntPoint;
}

bool CWNetDirectory::Create(const char* strPath)
{
  CStdString strPath1;
  
  CURL url(strPath);

  strPath1 = Open(url);

  if(strPath1==StringUtils::EmptyString)
    return false;

  CUtil::AddFileToFolder(strPath1,CWIN32Util::GetLocalPath(strPath),strPath1);

  strPath1.Replace("/", "\\");

  if(::CreateDirectory(strPath1.c_str(), NULL))
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CWNetDirectory::Remove(const char* strPath)
{
  CStdString strPath1;
  
  CURL url(strPath);

  strPath1 = Open(url);

  if(strPath1==StringUtils::EmptyString)
    return false;

  CUtil::AddFileToFolder(strPath1,CWIN32Util::GetLocalPath(strPath),strPath1);

  strPath1.Replace("/", "\\");
  return ::RemoveDirectory(strPath1) ? true : false;
}

bool CWNetDirectory::Exists(const char* strPath)
{
  CStdString strPath1;
  
  CURL url(strPath);

  strPath1 = Open(url);

  if(strPath1==StringUtils::EmptyString)
    return false;

  CUtil::AddFileToFolder(strPath1,CWIN32Util::GetLocalPath(strPath),strPath1);

  strPath1.Replace("/", "\\");
  
  DWORD attributes = GetFileAttributes(strPath1.c_str());
  if (FILE_ATTRIBUTE_DIRECTORY == attributes) return true;
  return false;
}
