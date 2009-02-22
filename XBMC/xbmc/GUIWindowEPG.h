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

#include "GUIWindow.h"
#include "DateTime.h"
#include "TVDatabase.h"
#include "EPG.h"
#include "GUIEPGGridContainer.h"
#include "GUIDialogProgress.h"
#include "PVRManager.h"

#include <vector>

class CFileItem;

class CGUIWindowEPG : public CGUIWindow, public IEPGObserver
{
public:
  CGUIWindowEPG(void);
  virtual ~CGUIWindowEPG(void);
  void OnWindowLoaded();
  void OnWindowUnload();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

  void OnInfo(CFileItem* pItem);

protected:
  virtual void OnChannelUpdated(unsigned channel);
  virtual void OnInitWindow();
  void GetGridData();
  void UpdateGrid();
  void Refresh();

  void ShowEPGInfo(CFileItem *item);
  void DisplayEmptyDatabaseMessage(bool bDisplay);
  bool m_bDisplayEmptyDatabaseMessage;

  CGUIDialogProgress* m_dlgProgress;

  int m_daysToDisplay;
  int m_curDaysOffset;
  CDateTime m_gridStart;
  CDateTime m_gridEnd;
  CDateTime m_dataEnd; // schedule data exists in the db up to this date
  int m_numChannels;
  bool m_favouritesOnly;

  CTVDatabase m_database;

  VECFILEITEMS* m_channels; // current list of channels in PVRManager
  VECCHANNELS* m_gridData; // current working set of guide data in PVRManager
  CGUIEPGGridContainer* m_guideGrid;
};
