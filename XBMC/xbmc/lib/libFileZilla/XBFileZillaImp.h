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

#ifndef __XBFILEZILLAIMP_H__
#define __XBFILEZILLAIMP_H__

#pragma once

#include "xbfilezilla.h"
#include "permissions.h"

#include "thread.h"
#include <vector>

class CFreeSpace 
{
public:
  CFreeSpace();

  ULARGE_INTEGER GetFreeSpace();

public:
  CStdString    mDrive;
  unsigned long mMinimumSpace; // in MB
  bool          mDisplay;
};

class CXBServer;

// singleton class
class CXBFileZillaImp : public CThread
{
public:

	virtual BOOL InitInstance();
	virtual DWORD ExitInstance();

  static CXBFileZillaImp* GetInstance();
  void DestructInstance();
  
  //////////////////////////////////////////////////
  // server runtime control

  bool Start(bool Wait = true);
  bool Stop();


  void SetConfigurationPath(LPCTSTR Path);
  LPCTSTR GetConfigurationPath();

  //////////////////////////////////////////////////
  // user administration methods

  XFSTATUS AddUser(LPCTSTR Name, CXFUser*& User);
  XFSTATUS RemoveUser(LPCTSTR Name);
  XFSTATUS GetUser(LPCTSTR Name, CXFUser*& User);
  XFSTATUS GetAllUsers(std::vector<CXFUser*>& UserVector);


  //////////////////////////////////////////////////
  // implementation specific methods
  CXBServer* GetServer();

  void SetCriticalOperationCallback(CriticalOperationCallback Callback);

  XFSTATUS LaunchXBE(CStdString& Filename);
  XFSTATUS Reboot();
  XFSTATUS Shutdown();

  
  XFSTATUS GetFileCRC(const CStdString& Filename, unsigned long& Crc);

  void SetCrcEnabled(bool CrcEnabled);
  bool GetCrcEnabled();
  void SetSfvEnabled(bool SfvEnabled);
  bool GetSfvEnabled();

  bool GetFreeSpacePrompt(unsigned ReplyCode, CStdString& Prompt);
  
  void     SetFreeSpace(LPCTSTR Drivename, bool DisplayAtPrompt);
  XFSTATUS GetFreeSpace(LPCTSTR Drivename, bool& DisplayAtPrompt);


  XFSTATUS ReadXBoxSettings();
  XFSTATUS WriteXBoxSettings();

protected:
  // returns the driveletter from a directory path
  // e.g. f:\media\movies will become f:\ 
  CStdString ConvertToDrivename(LPCTSTR Dirname);

protected:
	CXBServer* mServer;
  CStdString mConfigurationPath;
  CriticalOperationCallback mCriticalOperationCallback;
  bool mCrcEnabled;
  bool mSfvEnabled;
  bool mFreeSpacePromptEnabled;
  std::vector<CFreeSpace> mFreeSpaceDrives;

  static CCriticalSectionWrapper mXBoxSettingsCS;

private:
  CXBFileZillaImp();
	virtual ~CXBFileZillaImp();

  static CXBFileZillaImp* mInstance;
};


class CXFUserImp : public CXFUser
{
public:
  CXFUserImp();
  virtual ~CXFUserImp();

  virtual LPCTSTR GetName();
  virtual XFSTATUS SetName(LPCTSTR Name);
  virtual XFSTATUS SetPassword(LPCTSTR Password);

  virtual bool GetShortcutsEnabled();
  virtual void SetShortcutsEnabled(bool ShortcutsEnabled);
  virtual bool GetUseRelativePaths();
  virtual void SetUseRelativePaths(bool UseRelativePaths);
  virtual bool GetBypassUserLimit();
  virtual void SetBypassUserLimit(bool BypassUserLimit);
  virtual int  GetUserLimit();
  virtual void SetUserLimit(int UserLimit);
  virtual int  GetIPLimit();
  virtual void SetIPLimit(int IPLimit);

  virtual int  GetDownloadSpeedLimitType();
  virtual void SetDownloadSpeedLimitType(int DownloadSpeedLimitType);
  virtual int  GetDownloadSpeedLimit();
  virtual void SetDownloadSpeedLimit(int DownloadSpeedLimit);
  virtual int  GetUploadSpeedLimitType();
  virtual void SetUploadSpeedLimitType(int UploadSpeedLimitType);
  virtual int  GetUploadSpeedLimit();
  virtual void SetUploadSpeedLimit(int UploadSpeedLimit);

  virtual bool GetBypassServerDownloadSpeedLimit();
  virtual void SetBypassServerDownloadSpeedLimit(bool BypassServerDownloadSpeedLimit);
  virtual bool GetBypassServerUploadSpeedLimit();
  virtual void SetBypassServerUploadSpeedLimit(bool BypassServerUploadSpeedLimit);


  virtual DWORD    GetDirectoryPermissions(LPCTSTR DirName);
  virtual XFSTATUS SetDirectoryPermissions(LPCTSTR DirName, DWORD Permissions);

  virtual XFSTATUS AddDirectory(LPCTSTR DirName, DWORD Permissions);
  virtual XFSTATUS RemoveDirectory(LPCTSTR DirName);
  virtual XFSTATUS GetAllDirectories(std::vector<SXFDirectory>& DirVector);

  virtual XFSTATUS GetHomeDir(SXFDirectory& HomeDir);

  virtual XFSTATUS CommitChanges();

private:
  XFSTATUS Init(LPCTSTR Name);
  DWORD GetDirectoryPermissions(t_directory& Dir);
  void SetDirectoryPermissions(t_directory& Dir, DWORD Permissions);

  CUser mUser;
  friend class CXBFileZillaImp;
};


class CXFPermissions : public CPermissions
{
public:
  XFSTATUS AddUser(const CUser& user);
  XFSTATUS RemoveUser(LPCTSTR username);
  XFSTATUS ModifyUser(const CUser& user);
  BOOL UserExists(LPCTSTR username);

  unsigned GetUserCount();

  using CPermissions::GetUser;
  XFSTATUS GetUser(int index, t_user& user);

  CStdString GetUsername(int index);
};


#define XBFILEZILLA(arg) (CXBFileZillaImp::GetInstance()->##arg)

#endif // __XBFILEZILLAIMP_H__