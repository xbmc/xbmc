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
#include "IOSDOption.h"

namespace OSD
{
class COSDSubMenu
{
public:
  COSDSubMenu();
  COSDSubMenu(int iHeading, int iXpos, int iYpos);
  COSDSubMenu(const COSDSubMenu& submenu);
  const COSDSubMenu& operator = (const COSDSubMenu& submenu);

  virtual ~COSDSubMenu(void);
  COSDSubMenu* Clone() const;

  void Draw();
  void AddOption(const IOSDOption* option);
  bool OnAction(IExecutor& executor, const CAction& action);

  int GetX() const;
  void SetX(int X);

  int GetY() const;
  void SetY(int X) ;
  void SetValue(int iMessage, int iValue);
  void SetLabel(int iMessage, const CStdString& strLabel);
private:
  void Clear();
  typedef vector<IOSDOption*>::iterator ivecOptions;
  typedef vector<IOSDOption*>::const_iterator icvecOptions;
  vector<IOSDOption*> m_vecOptions;

  int m_iCurrentOption;
  int m_iXPos;
  int m_iYPos;
  int m_iHeading;
  bool m_bOptionSelected;
};
};
