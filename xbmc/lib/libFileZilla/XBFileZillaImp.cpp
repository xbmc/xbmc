/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// DebugTestApplication.cpp: implementation of the CXBFileZilla class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XBFileZillaImp.h"

#include "xbserver.h"
#include "Options.h"
#include "Permissions.h"
#include "misc\md5.h"
#include "misc\MarkupSTL.h"

#include "bsdsfv.h"
#include "utils/log.h"

#pragma warning (disable:4244)
#pragma warning (disable:4800)
CXBFileZillaImp* CXBFileZillaImp::mInstance = NULL;
CCriticalSectionWrapper CXBFileZillaImp::mXBoxSettingsCS;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXBFileZillaImp::CXBFileZillaImp()
{
  ASSERT(mInstance == NULL);
  mConfigurationPath = "T:\\";
  mServer = new CXBServer();
  mCriticalOperationCallback = NULL;
  mCrcEnabled = false;
  mSfvEnabled = false;
}

CXBFileZillaImp::~CXBFileZillaImp()
{
  delete mServer;
  mInstance = NULL;
}


BOOL CXBFileZillaImp::InitInstance()
{
  ReadXBoxSettings();
  if( mServer->Create() )
  {
    CLog::Log(LOGNOTICE, "XBFileZilla: Started");
    return true;
  }
  else
  {
    CLog::Log(LOGNOTICE, "XBFileZilla: Startup failed");
    return false;
  }

  /* set our normal thread proprity */
  SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL);
}

void CXBFileZillaImp::DestructInstance()
{
  delete mInstance;
}

CXBFileZillaImp* CXBFileZillaImp::GetInstance()
{
  if (!mInstance)
    mInstance = new CXBFileZillaImp();

  return mInstance;
}

DWORD CXBFileZillaImp::ExitInstance()
{
  // signal ftp server to stop
  //SendMessage(mServer->GetHwnd(), WM_CLOSE, 0, 0);
  SendMessage(mServer->GetHwnd(), WM_DESTROY, 0, 0);

  return 0;
}


bool CXBFileZillaImp::Stop()
{
  HANDLE handle = m_hThread;
  PostThreadMessage(WM_QUIT, 0, 0);

  bool bRet=(WAIT_FAILED != WaitForSingleObject(handle, INFINITE));

  RemoveMessageSinks();
  return bRet;
}

bool CXBFileZillaImp::Start(bool Wait)
{
  if( Wait )
  {
    if (!Create())
      return false;

    // wait for initinstance has been executed
    WaitForSingleObject(m_hEventStarted, INFINITE);
  }
  else
  {
    if (!Create(THREAD_PRIORITY_BELOW_NORMAL))
      return false;
  }

  return true;
}

CXBServer* CXBFileZillaImp::GetServer()
{
  if( m_hThread == NULL )
    return NULL;

  WaitForSingleObject(m_hEventStarted, INFINITE);

  return mServer;
}

void CXBFileZillaImp::SetConfigurationPath(LPCTSTR Path)
{
	if (Path)
		mConfigurationPath = Path;
	else
		mConfigurationPath.clear();
}

LPCTSTR CXBFileZillaImp::GetConfigurationPath()
{
  return mConfigurationPath.c_str();
}

XFSTATUS CXBFileZillaImp::AddUser(LPCTSTR Name, CXFUser*& User)
{
  if( m_hThread == NULL )
    return XFS_ERROR;

  if( WaitForSingleObject(m_hEventStarted, 5000) != WAIT_OBJECT_0 )
    return XFS_ERROR;

  CXFPermissions permissions;
  
  if (permissions.UserExists(Name))
  {
    User = NULL;
    return XFS_ALREADY_EXISTS;
  }

  CXFUserImp* user = new CXFUserImp();
  user->mUser.user = Name;

  User = user;
  return permissions.AddUser(user->mUser);
}

XFSTATUS CXBFileZillaImp::RemoveUser(LPCTSTR Name)
{
  if( m_hThread == NULL )
    return XFS_ERROR;

  WaitForSingleObject(m_hEventStarted, INFINITE);

  CXFPermissions permissions;
  return permissions.RemoveUser(Name);
}

XFSTATUS CXBFileZillaImp::GetUser(LPCTSTR Name, CXFUser*& User)
{
  if( m_hThread == NULL )
    return XFS_ERROR;

  if( WaitForSingleObject(m_hEventStarted, 5000) != WAIT_OBJECT_0 )
    return XFS_ERROR;

  CXFPermissions permissions;

  CXFUserImp* user = new CXFUserImp();
  XFSTATUS status = user->Init(Name);

  if (status != XFS_OK)
  {
    delete user;
    User = NULL;
    return status;
  }

  User = user;
  return XFS_OK;
}

XFSTATUS CXBFileZillaImp::GetAllUsers(std::vector<CXFUser*>& UserVector)
{
  if( m_hThread == NULL )
    return XFS_ERROR;

  if( WaitForSingleObject(m_hEventStarted, 5000) != WAIT_OBJECT_0 )
    return XFS_ERROR;

  CXFPermissions permissions;
  unsigned i;
  for (i = 0; i < UserVector.size(); i++)
    delete UserVector[i];

  UserVector.clear();
  
  CStdString username;

  for (i = 0; i < permissions.GetUserCount(); i++)
  {
    username = permissions.GetUsername(i);
    if (!username.IsEmpty())
    {
      CXFUserImp* user = new CXFUserImp();
      if (user->Init(username) == XFS_OK)
        UserVector.push_back(user);
      else
        delete user;
    }
  }

  return XFS_OK;
}

void CXBFileZillaImp::SetCriticalOperationCallback(CriticalOperationCallback Callback)
{
  mCriticalOperationCallback = Callback;
}

XFSTATUS CXBFileZillaImp::LaunchXBE(CStdString& Filename)
{
  if (mCriticalOperationCallback)
  {
    SXFCriticalOperation operation;
    operation.mOperation = SXFCriticalOperation::LaunchXBE;
    operation.mFilename = Filename.c_str();
    return mCriticalOperationCallback(operation);
  }
  else
    return XFS_NOT_IMPLEMENTED;
}

//void ;


XFSTATUS CXBFileZillaImp::Reboot()
{
  if (mCriticalOperationCallback)
  {
    SXFCriticalOperation operation;
    operation.mOperation = SXFCriticalOperation::Reboot;
    operation.mFilename = NULL;
    return mCriticalOperationCallback(operation);
  }
  else
    return XFS_NOT_IMPLEMENTED;
}

XFSTATUS CXBFileZillaImp::Shutdown()
{
  if (mCriticalOperationCallback)
  {
    SXFCriticalOperation operation;
    operation.mOperation = SXFCriticalOperation::Shutdown;
    operation.mFilename = NULL;
    return mCriticalOperationCallback(operation);
  }
  else
    return XFS_NOT_IMPLEMENTED;
}

bool CXBFileZillaImp::GetFreeSpacePrompt(unsigned ReplyCode, CStdString& Prompt)
{
  Prompt.Format(_T("%d- Free space: "), ReplyCode);

  for (unsigned i = 0; i < mFreeSpaceDrives.size(); ++i)
    if (mFreeSpaceDrives[i].mDisplay)
    {
      ULARGE_INTEGER freespace = mFreeSpaceDrives[i].GetFreeSpace();
      CStdString Unit;
      double freespacedouble = 0.0;
      if (freespace.QuadPart > (1024*1024*1024)) 
      {
        Unit = _T("GB");
        freespacedouble = (double)freespace.QuadPart / (1024*1024*1024);
      }
      else
      if (freespace.QuadPart > (1024*1024)) 
      {
        Unit = _T("MB");
        freespacedouble = (double)freespace.QuadPart / (1024*1024);
      }
      else
      if (freespace.QuadPart > 1024) 
      {
        Unit = _T("KB");
        freespacedouble = (double)freespace.QuadPart / 1024;
      }
      else
      {
        Unit = _T("bytes");
        freespacedouble = freespace.QuadPart;
      }

      Prompt.AppendFormat(_T("[%s %.2f %s] "), mFreeSpaceDrives[i].mDrive.c_str(), freespacedouble, Unit.c_str());
    }

  return !mFreeSpaceDrives.empty();
}


XFSTATUS CXBFileZillaImp::GetFileCRC(const CStdString& Filename, unsigned long& Crc)
{
  Crc = 0;

  if (!::GetFileCRC(Filename.c_str(), Crc))
    return XFS_ERROR;
  
  return XFS_OK;
}

void CXBFileZillaImp::SetCrcEnabled(bool enable)
{
  mCrcEnabled = enable;
  WriteXBoxSettings();
}

bool CXBFileZillaImp::GetCrcEnabled()
{
  return mCrcEnabled;
}

void CXBFileZillaImp::SetSfvEnabled(bool enable)
{
  mSfvEnabled = enable;
  WriteXBoxSettings();
}

bool CXBFileZillaImp::GetSfvEnabled()
{
  return mSfvEnabled;
}


CStdString CXBFileZillaImp::ConvertToDrivename(LPCTSTR Dirname)
{
  CStdString retval = Dirname;
  retval.ToUpper();
  retval.Trim();
  retval.TrimRight(_T("\\"));
  int index = retval.Find(_T("\\"));
  if (index != -1)
    retval = retval.Left(index);
  retval += _T("\\");
  return retval;
}

void CXBFileZillaImp::SetFreeSpace(LPCTSTR Drivename, bool DisplayAtPrompt)
{
  CStdString drive = ConvertToDrivename(Drivename);
  for (int i = 0; i < mFreeSpaceDrives.size(); i++)
    if (!mFreeSpaceDrives[i].mDrive.CompareNoCase(drive))
    {
      mFreeSpaceDrives[i].mDisplay = DisplayAtPrompt;
      break;
    }

  if (i == mFreeSpaceDrives.size())
  {
    CFreeSpace freespace;
    freespace.mDisplay = DisplayAtPrompt;
    freespace.mDrive = drive;
    mFreeSpaceDrives.push_back(freespace);
  }
  WriteXBoxSettings();
}

XFSTATUS CXBFileZillaImp::GetFreeSpace(LPCTSTR Drivename, bool& DisplayAtPrompt)
{
  CStdString drive = ConvertToDrivename(Drivename);
  for (int i = 0; i < mFreeSpaceDrives.size(); i++)
    if (!mFreeSpaceDrives[i].mDrive.CompareNoCase(drive))
    {
      DisplayAtPrompt = mFreeSpaceDrives[i].mDisplay;
      return XFS_OK;
    }

  return XFS_NOT_FOUND;
}

/*
<XBFileZilla>
  <Option Name="CrcEnabled">1</Option>
  <Option Name="SfvEnabled">1</Option>
  <FreeSpace>
    <Drive>
       <Name>c:</Name>
       <Minimum>10</Minimum>
       <Display>1</Display>
    </Drive>
    <Drive>
       <Name>e:</Name>
       <Minimum>200</Minimum>
       <Display>1</Display>
    </Drive>
    <Drive>
       <Name>f:</Name>
       <Minimum>500</Minimum>
       <Display>1</Display>
    </Drive>
  </FreeSpace>
</XBFileZilla>
*/
XFSTATUS CXBFileZillaImp::ReadXBoxSettings()
{
  mXBoxSettingsCS.Lock();

  mFreeSpaceDrives.clear();
  mSfvEnabled = false;
  mCrcEnabled = false;

  
  CMarkupSTL *pXML=COptions::GetXML();
	if (pXML)
	{
		if (!pXML->FindChildElem(_T("XBFileZilla")))
      if (!COptions::FreeXML(pXML))
      {
        mXBoxSettingsCS.Unlock();
        return XFS_ERROR;
      }
      else
      {
        mXBoxSettingsCS.Unlock();
        return XFS_NOT_FOUND;
      }

		pXML->IntoElem();

  
    while (pXML->FindChildElem())
    {
      CStdString tag = pXML->GetChildTagName();

      if (!tag.CompareNoCase(_T("Option")))
      {
        CStdString value = pXML->GetChildData();
        CStdString name  = pXML->GetChildAttrib( _T("Name") );
        if (name == _T("SfvEnabled"))
          mSfvEnabled = _ttoi(value);
        else
        if (name == _T("CrcEnabled"))
          mCrcEnabled = _ttoi(value);
      }
      else
      if (!tag.CompareNoCase(_T("FreeSpace")))
      {
        pXML->IntoElem();

        while (pXML->FindChildElem())
        {
          tag = pXML->GetChildTagName();
          if (!tag.CompareNoCase(_T("Drive")))
          {
            pXML->IntoElem();
            CFreeSpace freespace;

            while (pXML->FindChildElem())
            {
              tag = pXML->GetChildTagName();
              if (!tag.CompareNoCase(_T("Name")))
                freespace.mDrive = ConvertToDrivename(pXML->GetChildData());
              else
              if (!tag.CompareNoCase(_T("Minimum")))
              {
                CStdString value = pXML->GetChildData();
                freespace.mMinimumSpace = _ttoi(value);
              }
              else
              if (!tag.CompareNoCase(_T("Display")))
              {
                CStdString value = pXML->GetChildData();
                freespace.mDisplay = _ttoi(value);
              }
            }

            if (!freespace.mDrive.IsEmpty())
              mFreeSpaceDrives.push_back(freespace);
            pXML->OutOfElem();
          }
        }
  
        pXML->OutOfElem();
      }
    }

		if (!COptions::FreeXML(pXML))
    {
      mXBoxSettingsCS.Unlock();
			return XFS_ERROR;
    }
	}
	else
  {
    mXBoxSettingsCS.Unlock();
		return XFS_ERROR;
  }

  mXBoxSettingsCS.Unlock();
	return XFS_OK;
}


XFSTATUS CXBFileZillaImp::WriteXBoxSettings()
{
  mXBoxSettingsCS.Lock();
  CMarkupSTL *pXML=COptions::GetXML();
	if (pXML)
	{
    pXML->ResetPos();
		if (pXML->FindChildElem(_T("XBFileZilla")))
      pXML->RemoveChildElem();

    pXML->AddChildElem(_T("XBFileZilla"));
		pXML->IntoElem();

    pXML->AddChildElem(_T("Option"), mCrcEnabled?_T("1"):_T("0"));
	  pXML->AddChildAttrib(_T("Name"), _T("CrcEnabled"));
    pXML->AddChildElem(_T("Option"), mSfvEnabled?_T("1"):_T("0"));
	  pXML->AddChildAttrib(_T("Name"), _T("SfvEnabled"));

    pXML->AddChildElem(_T("FreeSpace"));
    pXML->IntoElem();
    for (unsigned i = 0; i < mFreeSpaceDrives.size(); i++)
    {
      pXML->AddChildElem(_T("Drive"));
      pXML->IntoElem();
      pXML->AddChildElem(_T("Name"), mFreeSpaceDrives[i].mDrive);
      CStdString str;
      str.Format(_T("%u"), mFreeSpaceDrives[i].mMinimumSpace);
      pXML->AddChildElem(_T("Minimum"), str);
      pXML->AddChildElem(_T("Display"), mFreeSpaceDrives[i].mDisplay?_T("1"):_T("0"));
      pXML->OutOfElem();
    }
    pXML->OutOfElem();
    pXML->OutOfElem();
    if (!COptions::FreeXML(pXML))
    {
      mXBoxSettingsCS.Unlock();
			return XFS_ERROR;
    }
  }
  else
  {
    mXBoxSettingsCS.Unlock();
    return XFS_ERROR;
  }

  mXBoxSettingsCS.Unlock();
	return XFS_OK;
}


//////////////////////////////////////////////////////////////////////////////////////
// CXFServerSettings

void CXFServerSettings::SetServerPort(int ServerPort)
{
  XBFILEZILLA(GetServer())->SetServerPort(ServerPort);
}


int CXFServerSettings::GetServerPort()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_SERVERPORT);
}


void CXFServerSettings::SetThreadNum(int ThreadNum)
{
  XBFILEZILLA(GetServer())->SetThreadNum(ThreadNum);
}

int CXFServerSettings::GetThreadNum()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_THREADNUM);
}

void CXFServerSettings::SetMaxUsers(int MaxUsers)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_MAXUSERS, MaxUsers);
}

int CXFServerSettings::GetMaxUsers()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_MAXUSERS);
}


void CXFServerSettings::SetTimeout(int Timeout)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_TIMEOUT, Timeout);
}

int CXFServerSettings::GetTimeout()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_TIMEOUT);
}


void CXFServerSettings::SetNoTransferTimeout(int NoTransferTimeout)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_NOTRANSFERTIMEOUT, NoTransferTimeout);
}

int CXFServerSettings::GetNoTransferTimeout()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_NOTRANSFERTIMEOUT);
}


void CXFServerSettings::SetInFxp(int InFxp)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_INFXP, InFxp);
}

int CXFServerSettings::GetInFxp()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_INFXP);
}


void CXFServerSettings::SetOutFxp(int OutFxp)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_OUTFXP, OutFxp);
}

int CXFServerSettings::GetOutFxp()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_OUTFXP);
}


void CXFServerSettings::SetNoInFxpStrict(int NoInFxpStrict)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_NOINFXPSTRICT, NoInFxpStrict);
}

int CXFServerSettings::GetNoInFxpStrict()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_NOINFXPSTRICT);
}


void CXFServerSettings::SetNoOutFxpStrict(int NoOutFxpStrict)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_NOOUTFXPSTRICT, NoOutFxpStrict);
}

int CXFServerSettings::GetNoOutFxpStrict()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_NOOUTFXPSTRICT);
}


void CXFServerSettings::SetLoginTimeout(int LoginTimeout)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_LOGINTIMEOUT, LoginTimeout);
}

int CXFServerSettings::GetLoginTimeout()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_LOGINTIMEOUT);
}


void CXFServerSettings::SetLogShowPass(int LogShowPass)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_LOGSHOWPASS, LogShowPass);
}

int CXFServerSettings::GetLogShowPass()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_LOGSHOWPASS);
}

void CXFServerSettings::SetCustomPasvIpType(int CustomPasvIpType)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_CUSTOMPASVIPTYPE, CustomPasvIpType);
  //XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_CUSTOMPASVENABLE, CustomPasvEnable);

}

int CXFServerSettings::GetCustomPasvIpType()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_CUSTOMPASVIPTYPE);
 // return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_CUSTOMPASVENABLE);
}

void CXFServerSettings::SetCustomPasvIP(LPCTSTR CustomPasvIP)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_CUSTOMPASVIP, CustomPasvIP);
}

LPCTSTR CXFServerSettings::GetCustomPasvIP()
{
  static CStdString sCustomPasvIP = _T("");
  sCustomPasvIP = XBFILEZILLA(GetServer())->GetOptions()->GetOption(OPTION_CUSTOMPASVIP);
  return sCustomPasvIP.c_str();
}


void CXFServerSettings::SetCustomPasvMinPort(int CustomPasvMinPort)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_CUSTOMPASVMINPORT, CustomPasvMinPort);
}

int CXFServerSettings::GetCustomPasvMinPort()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_CUSTOMPASVMINPORT);
}


void CXFServerSettings::SetCustomPasvMaxPort(int CustomPasvMaxPort)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_CUSTOMPASVMAXPORT, CustomPasvMaxPort);
}

int CXFServerSettings::GetCustomPasvMaxPort()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_CUSTOMPASVMAXPORT);
}


void CXFServerSettings::SetWelcomeMessage(LPCTSTR WelcomeMessage)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_WELCOMEMESSAGE, WelcomeMessage);
}

LPCTSTR CXFServerSettings::GetWelcomeMessage()
{
  static CStdString sWelcomeMessage = _T("");
  sWelcomeMessage = XBFILEZILLA(GetServer())->GetOptions()->GetOption(OPTION_WELCOMEMESSAGE);
  return sWelcomeMessage.c_str();
}


void CXFServerSettings::SetAdminPort(int AdminPort)
{
  XBFILEZILLA(GetServer())->SetAdminPort(AdminPort);
}

int CXFServerSettings::GetAdminPort()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_ADMINPORT);
}


void CXFServerSettings::SetAdminPass(LPCTSTR AdminPass)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_ADMINPASS, AdminPass);
}

LPCTSTR CXFServerSettings::GetAdminPass()
{
  static CStdString sAdminPass = _T("");
  sAdminPass = XBFILEZILLA(GetServer())->GetOptions()->GetOption(OPTION_ADMINPASS);
  return sAdminPass.c_str();
}


void CXFServerSettings::SetAdminIPBindings(LPCTSTR AdminIPBindings)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_ADMINIPBINDINGS, AdminIPBindings);
}

LPCTSTR CXFServerSettings::GetAdminIPBindings()
{
  static CStdString sAdminIPBindings = _T("");
  sAdminIPBindings = XBFILEZILLA(GetServer())->GetOptions()->GetOption(OPTION_ADMINIPBINDINGS);
  return sAdminIPBindings.c_str();
}


void CXFServerSettings::SetAdminIPAddresses(LPCTSTR AdminIPAddresses)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_ADMINIPADDRESSES, AdminIPAddresses);
}

LPCTSTR CXFServerSettings::GetAdminIPAddresses()
{
  static CStdString sAdminIPAddresses = _T("");
  sAdminIPAddresses = XBFILEZILLA(GetServer())->GetOptions()->GetOption(OPTION_ADMINIPADDRESSES);
  return sAdminIPAddresses.c_str();
}

void CXFServerSettings::SetEnableLogging(int EnableLogging)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_ENABLELOGGING, EnableLogging);
}

int CXFServerSettings::GetEnableLogging()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_ENABLELOGGING);
}


void CXFServerSettings::SetLogLimitSize(int LogLimitSize)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_LOGLIMITSIZE, LogLimitSize);
}

int CXFServerSettings::GetLogLimitSize()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_LOGLIMITSIZE);
}


void CXFServerSettings::SetLogType(int LogType)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_LOGTYPE, LogType);
}

int CXFServerSettings::GetLogType()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_LOGTYPE);
}


void CXFServerSettings::SetLogDeleteTime(int LogDeleteTime)
{
  XBFILEZILLA(GetServer())->GetOptions()->SetOption(OPTION_LOGDELETETIME, LogDeleteTime);
}

int CXFServerSettings::GetLogDeleteTime()
{
  return XBFILEZILLA(GetServer())->GetOptions()->GetOptionVal(OPTION_LOGDELETETIME);
}

void CXFServerSettings::SetCrcEnabled(bool CrcEnabled)
{
  XBFILEZILLA(SetCrcEnabled(CrcEnabled));
}

bool CXFServerSettings::GetCrcEnabled()
{
  return XBFILEZILLA(GetCrcEnabled());
}
 
void CXFServerSettings::SetSfvEnabled(bool SfvEnabled)
{
  XBFILEZILLA(SetSfvEnabled(SfvEnabled));
}

bool CXFServerSettings::GetSfvEnabled()
{
  return XBFILEZILLA(GetSfvEnabled());
}

void CXFServerSettings::SetFreeSpace(LPCTSTR Drivename, bool DisplayAtPrompt)
{
  XBFILEZILLA(SetFreeSpace(Drivename, DisplayAtPrompt));
}

XFSTATUS CXFServerSettings::GetFreeSpace(LPCTSTR Drivename, bool& DisplayAtPrompt)
{
  return XBFILEZILLA(GetFreeSpace(Drivename, DisplayAtPrompt));
}

/////////////////////////////////////////////////////////////////////////////////////
// CXFUserImp


CXFUserImp::CXFUserImp()
{
  mUser.user = _T("");
  mUser.nBypassUserLimit = FALSE;
  mUser.nLnk = FALSE;
  mUser.nRelative = TRUE;
  mUser.nIpLimit = 0;
  mUser.nUserLimit = 0;
  mUser.password = _T("");
}

CXFUserImp::~CXFUserImp()
{
}

XFSTATUS CXFUserImp::Init(LPCTSTR Name)
{
  CXFPermissions permissions;

  if (permissions.GetUser(Name, mUser))
    return XFS_OK;
  
  return XFS_NOT_FOUND;
}

LPCTSTR CXFUserImp::GetName()
{
  return mUser.user.c_str();
}

XFSTATUS CXFUserImp::SetName(LPCTSTR Name)
{
  if (mUser.user == Name)
    return XFS_OK;

  CXFPermissions permissions;
  CUser user;
  if (permissions.GetUser(Name, user))
    return XFS_ALREADY_EXISTS;

  if (permissions.RemoveUser(mUser.user) != XFS_OK)
  {
    // todo: log/notify ?
  }

  mUser.user = Name; 
 
  return permissions.AddUser(mUser);
}

XFSTATUS CXFUserImp::SetPassword(LPCTSTR Password)
{
  if (_tcslen(Password) == 0)
    return XFS_INVALID_PARAMETERS;

  const char *tmp = Password;
  MD5 md5;
  md5.update((unsigned char *)tmp, _tcslen(Password));
  md5.finalize();
  char *res=md5.hex_digest();
  CStdString hash = res;
  delete [] res;
  mUser.password = hash;
  return XFS_OK;
}

bool CXFUserImp::GetShortcutsEnabled()
{
  return mUser.nLnk;
}

void CXFUserImp::SetShortcutsEnabled(bool ShortcutsEnabled)
{
  mUser.nLnk = ShortcutsEnabled;
}

bool CXFUserImp::GetUseRelativePaths()
{
  return mUser.nRelative;
}

void CXFUserImp::SetUseRelativePaths(bool UseRelativePaths)
{
  mUser.nRelative = UseRelativePaths;
}

bool CXFUserImp::GetBypassUserLimit()
{
  return mUser.nBypassUserLimit;
}

void CXFUserImp::SetBypassUserLimit(bool BypassUserLimit)
{
  mUser.nBypassUserLimit = BypassUserLimit;
}

int  CXFUserImp::GetUserLimit()
{
  return mUser.nUserLimit;
}

void CXFUserImp::SetUserLimit(int UserLimit)
{
  mUser.nUserLimit = UserLimit;
}

int  CXFUserImp::GetIPLimit()
{
  return mUser.nIpLimit;
}

void CXFUserImp::SetIPLimit(int IPLimit)
{
  mUser.nIpLimit = IPLimit;
}


DWORD CXFUserImp::GetDirectoryPermissions(t_directory& Dir)
{
  DWORD permission = 0;

  permission |= Dir.bFileRead   ? XBFILE_READ   : 0;
  permission |= Dir.bFileWrite  ? XBFILE_WRITE  : 0;
  permission |= Dir.bFileDelete ? XBFILE_DELETE : 0;
  permission |= Dir.bFileAppend ? XBFILE_APPEND : 0;
  permission |= Dir.bDirCreate  ? XBDIR_CREATE  : 0;
  permission |= Dir.bDirDelete  ? XBDIR_DELETE  : 0;
  permission |= Dir.bDirList    ? XBDIR_LIST    : 0;
  permission |= Dir.bDirSubdirs ? XBDIR_SUBDIRS : 0;
  permission |= Dir.bIsHome     ? XBDIR_HOME    : 0;

  return permission;
}

DWORD CXFUserImp::GetDirectoryPermissions(LPCTSTR DirName)
{
  for (unsigned i = 0; i < mUser.permissions.size(); i++)
    if (!mUser.permissions[i].dir.CompareNoCase(DirName))
      return GetDirectoryPermissions(mUser.permissions[i]);

  return XBPERMISSION_DENIED;
}

void CXFUserImp::SetDirectoryPermissions(t_directory& Dir, DWORD Permissions)
{
  Dir.bFileRead   = Permissions & XBFILE_READ;
  Dir.bFileWrite  = Permissions & XBFILE_WRITE;
  Dir.bFileDelete = Permissions & XBFILE_DELETE;
  Dir.bFileAppend = Permissions & XBFILE_APPEND;
  Dir.bDirCreate  = Permissions & XBDIR_CREATE;
  Dir.bDirDelete  = Permissions & XBDIR_DELETE;
  Dir.bDirList    = Permissions & XBDIR_LIST;
  Dir.bDirSubdirs = Permissions & XBDIR_SUBDIRS;
  Dir.bIsHome     = Permissions & XBDIR_HOME;
}

XFSTATUS CXFUserImp::SetDirectoryPermissions(LPCTSTR DirName, DWORD Permissions)
{
  for (unsigned i = 0; i < mUser.permissions.size(); i++)
    if (!mUser.permissions[i].dir.CompareNoCase(DirName))
    {
      if (Permissions & XBDIR_HOME)
        for (unsigned j = 0; j < mUser.permissions.size(); j++)
          mUser.permissions[j].bIsHome = false;

      SetDirectoryPermissions(mUser.permissions[i], Permissions);

      return XFS_OK;
    }

  return XFS_NOT_FOUND;
}

XFSTATUS CXFUserImp::AddDirectory(LPCTSTR DirName, DWORD Permissions)
{
  for (unsigned i = 0; i < mUser.permissions.size(); i++)
    if (!mUser.permissions[i].dir.CompareNoCase(DirName))
      return XFS_ALREADY_EXISTS;

  t_directory newDir;
  newDir.dir = DirName;
  SetDirectoryPermissions(newDir, Permissions);
  if (Permissions & XBDIR_HOME)
	{
		mUser.homedir = newDir.dir;
		for (unsigned i = 0; i < mUser.permissions.size(); i++)
			mUser.permissions[i].bIsHome = false;
	}

  mUser.permissions.push_back(newDir);

  return XFS_OK;
}

XFSTATUS CXFUserImp::RemoveDirectory(LPCTSTR DirName)
{
  std::vector<t_directory>::iterator it;

  for (it = mUser.permissions.begin(); it != mUser.permissions.end(); ++it)
    if (!(*it).dir.CompareNoCase(DirName))
    {
      mUser.permissions.erase(it);
      return XFS_OK;
    }

  return XFS_NOT_FOUND;
}

XFSTATUS CXFUserImp::GetAllDirectories(std::vector<SXFDirectory>& DirVector)
{
  DirVector.clear();
  for (unsigned i = 0; i < mUser.permissions.size(); i++)
  {
    SXFDirectory dir;
    dir.mDirName = mUser.permissions[i].dir;
    dir.mPermissions = GetDirectoryPermissions(mUser.permissions[i]);
    DirVector.push_back(dir);
  }

  return XFS_OK;
}

XFSTATUS CXFUserImp::GetHomeDir(SXFDirectory& HomeDir)
{
  for (unsigned i = 0; i < mUser.permissions.size(); i++)
    if (mUser.permissions[i].bIsHome)
    {
      HomeDir.mDirName = mUser.permissions[i].dir;
      HomeDir.mPermissions = GetDirectoryPermissions(mUser.permissions[i]);
      return XFS_OK;
    }

  return XFS_NOT_FOUND;
}


XFSTATUS CXFUserImp::CommitChanges()
{
  CXFPermissions permissions;
  return permissions.ModifyUser(mUser);
}



////////////////////////////////////////////////////////////////////////////

unsigned CXFPermissions::GetUserCount()
{
  return m_sUsersList.size();
}

XFSTATUS CXFPermissions::GetUser(int index, t_user& user)
{
  if (index >= m_sUsersList.size())
    return XFS_INVALID_PARAMETERS;

  user = m_sUsersList[index];
  return XFS_OK;
}

CStdString CXFPermissions::GetUsername(int index)
{
  if (index >= m_sUsersList.size())
    return _T("");

  return m_sUsersList[index].user;
}

XFSTATUS CXFPermissions::AddUser(const CUser& user)
{
	//Update the account list
	m_sync.Lock();
	
  t_UsersList::iterator iter;
  for (iter = m_sUsersList.begin(); iter != m_sUsersList.end(); ++iter)
    if (!(*iter).user.CompareNoCase(user.user))
    {
      m_sync.Unlock();
      return XFS_ALREADY_EXISTS;
    }

	m_sUsersList.push_back(user);
		
	UpdateInstances();
	
	m_sync.Unlock();
	
	CMarkupSTL *pXML=COptions::GetXML();
	if (pXML)
	{
		pXML->FindChildElem(_T("Users"));
		pXML->IntoElem();
		
		//Save the user details
    pXML->AddChildElem(_T("User"));
    pXML->AddChildAttrib(_T("Name"), user.user);
    pXML->IntoElem();
    SetKey(pXML, "Pass", user.password);
    SetKey(pXML, "Resolve Shortcuts", user.nLnk?"1":"0");
    SetKey(pXML, "Relative", user.nRelative?"1":"0");
    SetKey(pXML, "Bypass server userlimit", user.nBypassUserLimit?"1":"0");
    CStdString str;
    str.Format(_T("%d"), user.nUserLimit);
    SetKey(pXML, "User Limit", str);
    str.Format(_T("%d"), user.nIpLimit);
    SetKey(pXML, "IP Limit", str);

    SavePermissions(pXML, user);
    pXML->OutOfElem();

		if (!COptions::FreeXML(pXML))
			return XFS_ERROR;
	}
	else
		return XFS_ERROR;

	return XFS_OK;
}

XFSTATUS CXFPermissions::RemoveUser(LPCTSTR username)
{
  //Update the account list
	m_sync.Lock();
	
  t_UsersList::iterator iter;
  for (iter = m_sUsersList.begin(); iter != m_sUsersList.end(); ++iter)
    if (!(*iter).user.CompareNoCase(username))
    {
      m_sUsersList.erase(iter);    
      UpdateInstances();
      break;
    }
	
	m_sync.Unlock();

  CMarkupSTL *pXML=COptions::GetXML();
	if (pXML)
	{
		pXML->FindChildElem(_T("Users"));
		pXML->IntoElem();
		
    bool found = false;

    while (pXML->FindChildElem(_T("User")) && !found)
    {
      if (pXML->GetChildAttrib(_T("Name")) == username)
      {
        found = true;
        pXML->RemoveChildElem();
      }
    }
		if (!COptions::FreeXML(pXML))
			return XFS_ERROR;
	}
	else
		return XFS_ERROR;

	return XFS_OK;
}


XFSTATUS CXFPermissions::ModifyUser(const CUser& user)
{
	RemoveUser(user.user);
  return AddUser(user);
}


BOOL CXFPermissions::UserExists(LPCTSTR username)
{
  t_UsersList::iterator iter;
  for (iter = m_sUsersList.begin(); iter != m_sUsersList.end(); ++iter)
    if (!(*iter).user.CompareNoCase(username))
      return TRUE;

  return FALSE;
}



/////////////////////////////////////////////////////////
// CFreeSpace

CFreeSpace::CFreeSpace()
: mDrive(_T(""))
, mDisplay(true)
, mMinimumSpace(0)
{
}

ULARGE_INTEGER CFreeSpace::GetFreeSpace()
{
  ULARGE_INTEGER FreeBytesAvailable;    // bytes available
  FreeBytesAvailable.QuadPart = 0;

  if (mDrive.IsEmpty())
    return FreeBytesAvailable;

  ULARGE_INTEGER TotalNumberOfBytes;    // bytes on disk
  ULARGE_INTEGER TotalNumberOfFreeBytes; // free bytes on disk

  if (GetDiskFreeSpaceEx(mDrive.c_str(), &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
    return FreeBytesAvailable;
  else 
  {
    FreeBytesAvailable.QuadPart = 0;
    return FreeBytesAvailable;
  }
}



/*
	SPEEDLIMITSLIST DownloadSpeedLimits, UploadSpeedLimits;
*/



bool CXFUserImp::GetBypassServerDownloadSpeedLimit()
{
  return mUser.nBypassServerDownloadSpeedLimit;
}

void CXFUserImp::SetBypassServerDownloadSpeedLimit(bool BypassServerDownloadSpeedLimit)
{
  mUser.nBypassServerDownloadSpeedLimit = BypassServerDownloadSpeedLimit;
}

bool CXFUserImp::GetBypassServerUploadSpeedLimit()
{
  return mUser.nBypassServerUploadSpeedLimit;
}

void CXFUserImp::SetBypassServerUploadSpeedLimit(bool BypassServerUploadSpeedLimit)
{
  mUser.nBypassServerUploadSpeedLimit = BypassServerUploadSpeedLimit;
}


int  CXFUserImp::GetDownloadSpeedLimitType()
{
  return mUser.nDownloadSpeedLimitType;
}

void CXFUserImp::SetDownloadSpeedLimitType(int DownloadSpeedLimitType)
{
  mUser.nDownloadSpeedLimitType = DownloadSpeedLimitType;
}

int  CXFUserImp::GetDownloadSpeedLimit()
{
  return mUser.nDownloadSpeedLimit;
}

void CXFUserImp::SetDownloadSpeedLimit(int DownloadSpeedLimit)
{
  mUser.nDownloadSpeedLimit = DownloadSpeedLimit;
}


int  CXFUserImp::GetUploadSpeedLimitType()
{
  return mUser.nUploadSpeedLimitType;
}

void CXFUserImp::SetUploadSpeedLimitType(int UploadSpeedLimitType)
{
  mUser.nUploadSpeedLimitType = UploadSpeedLimitType;
}

int  CXFUserImp::GetUploadSpeedLimit()
{
  return mUser.nUploadSpeedLimit;
}

void CXFUserImp::SetUploadSpeedLimit(int UploadSpeedLimit)
{
  mUser.nUploadSpeedLimit = UploadSpeedLimit;
}

