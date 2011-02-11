#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "guilib/GUIWindow.h"
#include "bluetooth/IBluetoothSyscall.h"

class CGUIWindowSettingsBluetooth :
      public CGUIWindow
{
public:
  CGUIWindowSettingsBluetooth(void);
  virtual ~CGUIWindowSettingsBluetooth(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void UpdateDevice(IBluetoothDevice *device);
  void RemoveDiscoveredDevice(const char *address);
  void RemoveDevice(const char *path);

protected:
  virtual void OnInitWindow();
  CFileItemList *m_listItems;
  bool m_active;

  int GetSelectedItem();
  void LoadList();
  void ClearListItems();
  void RefreshList();
  void OnItemSelected(int iItem);
};
