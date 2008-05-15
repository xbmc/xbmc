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
#include "OSDSubMenu.h"

namespace OSD
{
class COSDMenu
{
public:
  COSDMenu();
  COSDMenu(int iXpos, int iYpos);
  COSDMenu(const COSDMenu& menu);
  const COSDMenu& operator = (const COSDMenu& menu);

  virtual ~COSDMenu(void);
  COSDMenu* Clone();

  void AddSubMenu(const COSDSubMenu& submenu);
  void Draw();
  bool OnAction(IExecutor& executor, const CAction& action);

  int GetX() const;
  void SetX(int X);

  int GetY() const;
  void SetY(int Y) ;

  int GetSelectedMenu() const;
  void Clear();
  void SetValue(int iMessage, int iValue);
  void SetLabel(int iMessage, const CStdString& strLabel);
private:
  typedef vector<COSDSubMenu*>::iterator ivecSubMenus;
  typedef vector<COSDSubMenu*>::const_iterator icvecSubMenus;
  vector<COSDSubMenu*> m_vecSubMenus;

  int m_iCurrentSubMenu;
  int m_iXPos;
  int m_iYPos;
};
};
