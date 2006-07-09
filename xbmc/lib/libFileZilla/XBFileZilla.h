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

// TestApplication.h: interface for the CXBFileZilla class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __XBFILEZILLA_H__
#define __XBFILEZILLA_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <list>
#include	<tchar.h>

#define XFSTATUS DWORD

#define XFS_OK                  0x0000
#define XFS_ERROR               0x8000
#define XFS_NOT_IMPLEMENTED    (XFS_ERROR | 0x0001)
#define XFS_INVALID_PARAMETERS (XFS_ERROR | 0x0002)
#define XFS_ALREADY_EXISTS     (XFS_ERROR | 0x0003)
#define XFS_NOT_FOUND          (XFS_ERROR | 0x0004)
#define XFS_NOT_EXECUTABLE     (XFS_ERROR | 0x0005)
#define XFS_NOT_ENABLED        (XFS_ERROR | 0x0006)

typedef struct
{
  enum enumOperation {
    Reboot,
    Shutdown,
    LaunchXBE
  };

  enumOperation mOperation;
  LPCTSTR mFilename;
} SXFCriticalOperation;

typedef XFSTATUS (*CriticalOperationCallback)(SXFCriticalOperation CriticalOperation); // expected return values XFS_OK, XFS_NOT_FOUND or XFS_NOT_EXECUTABLE


typedef struct {
  enum enumFileTransferStatus{
    recvBegin, // recv: from ftpclient to xbox
    recvEnd,
    recvError,
    sendBegin, // send: from xbox to ftpclient
    sendEnd,
    sendError,
    unknownError
  }; 

  int mConnectionId;
  std::basic_string<TCHAR> mFilename;
  enumFileTransferStatus mStatus;
  bool mIsResumed;

} SXFTransferInfo;


typedef struct {
  int mId;
  std::basic_string<TCHAR> mUsername; // username is case insensitive
	std::basic_string<TCHAR> mIPAddress;
	unsigned int mPort;
} SXFConnection;


typedef enum { connectionAdd, connectionModify, connectionRemove } enumConnectionStatus;
// connectionAdd: connection has been made, but user has not yet logged in
// connectionModify: login details have changed, i.e. user has logged in or logged out
// connectionRemove: connection is broken

typedef enum { serverOk, serverError, userReceive, userSend } enumStatusSource;


class CXFNotificationClient
{
public:
  // OnShowStatus is the equivalent of the remote admin main output window
  // i.e. it receives all output text message xbfilezilla produces
  virtual void OnShowStatus(_int64 eventDate, LPCTSTR Msg, enumStatusSource Source) {};
  virtual void OnConnection(int ConnectionId, enumConnectionStatus ConnectionStatus) {};
  // don't store TransferInfo, it's a temporary object
  virtual void OnFileTransfer(SXFTransferInfo* TransferInfo) {};
  virtual void OnServerWillQuit() {};
  // OnIncSendCount and OnIncRecvCount will be called 10 times a second
  // the Count parameter is the total amount of bytes sent/received by all users together
  // in the last 1/10th of a second.
  virtual void OnIncSendCount(int Count) {};
  virtual void OnIncRecvCount(int Count) {};
};


// directory permissions
#define XBPERMISSION_DENIED 0x0000

#define XBFILE_READ    0x0001
#define XBFILE_WRITE   0x0002
#define XBFILE_DELETE  0x0004
#define XBFILE_APPEND  0x0008
#define XBDIR_DELETE   0x0010
#define XBDIR_CREATE   0x0020
#define XBDIR_LIST     0x0040
#define XBDIR_SUBDIRS  0x0080
#define XBDIR_HOME     0x0100
// todo: implement FILE_EXECUTE permission?

typedef struct {
  std::basic_string<TCHAR> mDirName; // file/directory names are case insensitive on the xbox
  DWORD mPermissions;
} SXFDirectory;


// abstract class
// instances of CXFUser are retrievd with CXBFileZilla::GetUser()
// in order to change the actual user data in the config file, call CXFUser::CommitChanges()
class CXFUser
{
public:
  virtual LPCTSTR GetName() = 0;
  virtual XFSTATUS SetName(LPCTSTR Name) = 0;
  virtual XFSTATUS SetPassword(LPCTSTR Password) = 0;

  virtual bool GetShortcutsEnabled() = 0;
  virtual void SetShortcutsEnabled(bool ShortcutsEnabled) = 0;
  virtual bool GetUseRelativePaths() = 0;
  virtual void SetUseRelativePaths(bool UseRelativePaths) = 0;
  virtual bool GetBypassUserLimit() = 0;
  virtual void SetBypassUserLimit(bool BypassUserLimit) = 0;
  virtual int  GetUserLimit() = 0;
  virtual void SetUserLimit(int UserLimit) = 0;
  virtual int  GetIPLimit() = 0;
  virtual void SetIPLimit(int IPLimit) = 0;


  virtual int  GetDownloadSpeedLimitType()= 0;
  virtual void SetDownloadSpeedLimitType(int DownloadSpeedLimitType)= 0;
  virtual int  GetDownloadSpeedLimit()= 0;
  virtual void SetDownloadSpeedLimit(int DownloadSpeedLimit)= 0;
  virtual int  GetUploadSpeedLimitType()= 0;
  virtual void SetUploadSpeedLimitType(int UploadSpeedLimitType)= 0;
  virtual int  GetUploadSpeedLimit()= 0;
  virtual void SetUploadSpeedLimit(int UploadSpeedLimit)= 0;
  virtual bool GetBypassServerDownloadSpeedLimit()= 0;
  virtual void SetBypassServerDownloadSpeedLimit(bool BypassServerDownloadSpeedLimit)= 0;
  virtual bool GetBypassServerUploadSpeedLimit()= 0;
  virtual void SetBypassServerUploadSpeedLimit(bool BypassServerUploadSpeedLimit)= 0;

  virtual DWORD    GetDirectoryPermissions(LPCTSTR Path) = 0;
  virtual XFSTATUS SetDirectoryPermissions(LPCTSTR Path, DWORD Permissions) = 0;

  virtual XFSTATUS AddDirectory(LPCTSTR DirName, DWORD Permissions) = 0;
  virtual XFSTATUS RemoveDirectory(LPCTSTR DirName) = 0;
  virtual XFSTATUS GetAllDirectories(std::vector<SXFDirectory>& DirVector) = 0;

  virtual XFSTATUS GetHomeDir(SXFDirectory& HomeDir) = 0;

  // invoke CommitChanges to change the user info in the config file
  virtual XFSTATUS CommitChanges() = 0;
};

//////////////////////////////////////////////////
// general administration methods
class CXFServerSettings
{
public:
  void SetServerPort(int ServerPort);
  int  GetServerPort();

  void SetThreadNum(int ThreadNum);
  int  GetThreadNum();

  void SetMaxUsers(int MaxUsers);
  int  GetMaxUsers();

  void SetTimeout(int Timeout);
  int  GetTimeout();

  void SetNoTransferTimeout(int NoTransferTimeout);
  int  GetNoTransferTimeout();

  void SetInFxp(int InFxp);
  int  GetInFxp();

  void SetOutFxp(int OutFxp);
  int  GetOutFxp();

  void SetNoInFxpStrict(int NoInFxpStrict);
  int  GetNoInFxpStrict();

  void SetNoOutFxpStrict(int NoOutFxpStrict);
  int  GetNoOutFxpStrict();

  void SetLoginTimeout(int LoginTimeout);
  int  GetLoginTimeout();

  void SetLogShowPass(int LogShowPass);
  int  GetLogShowPass();

  void SetCustomPasvIpType(int CustomPasvIpType);
  int  GetCustomPasvIpType();

  void    SetCustomPasvIP(LPCTSTR CustomPasvIP);
  LPCTSTR GetCustomPasvIP();

  void SetCustomPasvMinPort(int CustomPasvMinPort);
  int  GetCustomPasvMinPort();

  void SetCustomPasvMaxPort(int CustomPasvMaxPort);
  int  GetCustomPasvMaxPort();
  
  void    SetWelcomeMessage(LPCTSTR WelcomeMessage);
  LPCTSTR GetWelcomeMessage();
  
  void SetAdminPort(int AdminPort);
  int  GetAdminPort();
  
  void    SetAdminPass(LPCTSTR AdminPass);
  LPCTSTR GetAdminPass();

  void    SetAdminIPBindings(LPCTSTR AdminIPBindings);
  LPCTSTR GetAdminIPBindings();
  
  void    SetAdminIPAddresses(LPCTSTR AdminIPAddresses);
  LPCTSTR GetAdminIPAddresses();
  
  void SetEnableLogging(int EnableLogging);
  int  GetEnableLogging();
  
  void SetLogLimitSize(int LogLimitSize);
  int  GetLogLimitSize();
  
  void SetLogType(int LogType);
  int  GetLogType();
  
  void SetLogDeleteTime(int LogDeleteTime);
  int  GetLogDeleteTime();

  //////////////////////////////////////////////////////////////
  // xbfilezilla specific settings. i.e. not part of filezilla server
  // 
  // if CrcEnabled then for every file, that is successfully uploaded to the xbox,
  // the crc32 will be calculated, according to the sfv crc algorithm
  void SetCrcEnabled(bool CrcEnabled);
  bool GetCrcEnabled();
  //
  // if SfvEnabled then every file that is successfully uploaded to the xbox
  // will be crc-checked with a sfv file in the same directory.
  // if the file's crc does not correspond with the one in the sfv file, the file will
  // be renamed.
  // note: files uploaded to a directory before a sfv file is uploaded, will not be checked.
  void SetSfvEnabled(bool SfvEnabled);
  bool GetSfvEnabled();
  //
  // Free space reporting at ftp prompt 
  // if enabled, 
  // drivename must be a valid directory path, it will be shortened to the root dir
  // e.g. f:\media\movies will become f:\ 
  void SetFreeSpace(LPCTSTR Drivename, bool DisplayAtPrompt);
  XFSTATUS GetFreeSpace(LPCTSTR Drivename, bool& DisplayAtPrompt);
};



class CXBFileZilla
{
public:
  //////////////////////////////////////////////////
  // construction/destruction

  CXBFileZilla(LPCTSTR Path = _T("T:\\"));
	virtual ~CXBFileZilla();

  void SetConfigurationPath(LPCTSTR Path);
  LPCTSTR GetConfigurationPath();

  //////////////////////////////////////////////////
  // server runtime control

  bool Start(bool Wait = true);
  bool Stop();

  //////////////////////////////////////////////////
  // notifications
  //
  XFSTATUS AddNotificationClient(CXFNotificationClient* Client);
  XFSTATUS RemoveNotificationClient(CXFNotificationClient* Client);
  
  void SetCriticalOperationCallback(CriticalOperationCallback Callback);

  //////////////////////////////////////////////////
  // user administration methods

  XFSTATUS AddUser(LPCTSTR Name, CXFUser*& User);
  XFSTATUS RemoveUser(LPCTSTR Name);
  XFSTATUS GetUser(LPCTSTR Name, CXFUser*& User);
  XFSTATUS GetAllUsers(std::vector<CXFUser*>& UserVector);
  
  //////////////////////////////////////////////////
  // connection methods
  XFSTATUS CloseConnection(int ConnectionId); // i.e. kick user
  XFSTATUS GetAllConnections(std::vector<SXFConnection>& ConnectionVector);
  XFSTATUS GetConnection(int ConnectionId, SXFConnection& Connection);
  XFSTATUS GetNoConnections(void);

  CXFServerSettings mSettings;
};


#endif // __XBFILEZILLA_H__
