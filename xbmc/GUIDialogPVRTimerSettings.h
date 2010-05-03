#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "DateTime.h"
#include "GUIDialogSettings.h"
#include "GUIListItem.h"

class CFileItem;

class CGUIDialogPVRTimerSettings : public CGUIDialogSettings
{
public:
  CGUIDialogPVRTimerSettings(void);
  virtual ~CGUIDialogPVRTimerSettings(void) {}
  void SetTimer(CFileItem *item);
  bool GetOK() { return !m_cancelled; }

protected:
  virtual void CreateSettings();
  virtual void OnSettingChanged(SettingInfo &setting);
  virtual void OnOkay();
  virtual void OnCancel() { m_cancelled = true; }

  SYSTEMTIME      timerStartTime;
  SYSTEMTIME      timerEndTime;
  CStdString      timerStartTimeStr;
  CStdString      timerEndTimeStr;
  int             m_tmp_iFirstDay;;
  int             m_tmp_day;

  CFileItem      *m_timerItem;
  bool            m_cancelled;
};

