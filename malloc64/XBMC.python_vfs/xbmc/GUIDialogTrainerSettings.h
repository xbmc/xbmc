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
#include "ProgramDatabase.h"

class CTrainer;

class CGUIDialogTrainerSettings : public CGUIDialogSettings
{
public:
  CGUIDialogTrainerSettings(void);
  virtual ~CGUIDialogTrainerSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForTitle(unsigned int iTitleId, CProgramDatabase* database);
protected:
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  virtual void OnCancel();
  void OnSettingChanged(unsigned int setting);

  void AddBool(unsigned int id, const CStdString& strLabel, unsigned char* on);

  std::vector<CStdString> m_vecOptions;
  int m_iTrainer;
  int m_iOldTrainer;
  unsigned int m_iTitleId;
  std::vector<CTrainer*> m_vecTrainers;
  CProgramDatabase* m_database;
  bool m_bNeedSave;
  bool m_bCanceled;
  CStdString m_strActive; // active trainer at start - to save db work
};

