#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogSettings.h"
#include "GUIPassword.h"

class CGUIDialogLockSettings : public CGUIDialogSettings
{
public:
  CGUIDialogLockSettings(void);
  virtual ~CGUIDialogLockSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  static bool ShowAndGetLock(LockType& iLockMode, CStdString& strPassword, int iHeader=20091);
  static bool ShowAndGetLock(LockType& iLockMode, CStdString& strPassword, bool& bLockMusic, bool& bLockVideo, bool& bLockPictures, bool& bLockPrograms, bool& bLockFiles, bool& bLockSettings, int iButtonLabel=20091,bool bConditional=false, bool bDetails=true);
  static bool ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL);
protected:
  virtual void OnCancel();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  virtual void OnSettingChanged(SettingInfo &setting);
  void EnableDetails(bool bEnable);

  LockType m_iLock;
  CStdString m_strLock;
  CStdString m_strUser;
  CStdString m_strURL;
  bool m_bChanged;
  bool m_bDetails;
  bool m_bConditionalDetails;
  bool m_bGetUser;
  int m_iButtonLabel;
  bool m_bLockMusic; 
  bool m_bLockVideo;
  bool m_bLockPictures;
  bool m_bLockPrograms;
  bool m_bLockFiles;
  bool m_bLockSettings;
};

