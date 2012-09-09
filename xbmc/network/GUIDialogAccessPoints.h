#ifndef GUI_DIALOG_ACCES_POINTS
#define GUI_DIALOG_ACCES_POINTS

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>
#include "guilib/GUIDialog.h"
#include "Network.h"

class CFileItemList;

class CGUIDialogAccessPoints : public CGUIDialog
{
public:
  CGUIDialogAccessPoints(void);
  virtual ~CGUIDialogAccessPoints(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  void SetInterfaceName(CStdString interfaceName);
  CStdString GetSelectedAccessPointEssId();
  EncMode GetSelectedAccessPointEncMode();
  bool WasItemSelected();

private:
  std::vector<NetworkAccessPoint> m_aps;
  CStdString m_interfaceName;
  CStdString m_selectedAPEssId;
  EncMode m_selectedAPEncMode;
  bool m_wasItemSelected;
  CFileItemList *m_accessPoints;
};

#endif
