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
#include "stdafx.h"
#include "osdmenu.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace OSD;

COSDMenu::COSDMenu()
{
  m_iCurrentSubMenu = 0;
  m_iXPos = 0;
  m_iYPos = 0;
}

COSDMenu::COSDMenu(int iXpos, int iYpos)
{
  m_iCurrentSubMenu = 0;
  m_iXPos = iXpos;
  m_iYPos = iYpos;
}

COSDMenu::COSDMenu(const COSDMenu& menu)
{
  *this = menu;
}

COSDMenu::~COSDMenu(void)
{
  Clear();
}
void COSDMenu::Clear()
{
  ivecSubMenus i = m_vecSubMenus.begin();
  while (i != m_vecSubMenus.end())
  {
    COSDSubMenu* pSubMenu = *i;
    delete pSubMenu;
    i = m_vecSubMenus.erase(i);
  }
  m_iCurrentSubMenu = 0;
}


COSDMenu* COSDMenu::Clone()
{
  return new COSDMenu(*this);
}


const COSDMenu& COSDMenu::operator = (const COSDMenu& menu)
{
  if (&menu == this) return * this;
  Clear();
  icvecSubMenus i = menu.m_vecSubMenus.begin();
  while (i != menu.m_vecSubMenus.end())
  {
    const COSDSubMenu* pSubMenu = *i;
    m_vecSubMenus.push_back ( pSubMenu->Clone() );
    ++i;
  }

  m_iCurrentSubMenu = menu.m_iCurrentSubMenu;
  m_iXPos = menu.m_iXPos;
  m_iYPos = menu.m_iYPos;
  return *this;
}


void COSDMenu::Draw()
{
  // g_graphicsContext.Get3DDevice()->SetSoftDisplayFilter(true);
  // g_graphicsContext.Get3DDevice()->SetFlickerFilter(5);

  if (m_iCurrentSubMenu < 0 || m_iCurrentSubMenu >= (int) m_vecSubMenus.size())
  {
    m_iCurrentSubMenu = 0;
    return ;
  }
  COSDSubMenu* pSubMenu = m_vecSubMenus[m_iCurrentSubMenu];
  pSubMenu->Draw();


  //  g_graphicsContext.Get3DDevice()->SetSoftDisplayFilter(g_stSettings.m_bSoften);
  // g_graphicsContext.Get3DDevice()->SetFlickerFilter(g_stSettings.m_bSoften ? 5 : 1);
}

bool COSDMenu::OnAction(IExecutor& executor, const CAction& action)
{
  if (m_iCurrentSubMenu < 0 || m_iCurrentSubMenu >= (int)m_vecSubMenus.size()) return false; // invalid choice.
  COSDSubMenu* pSubMenu = m_vecSubMenus[m_iCurrentSubMenu];


  if (pSubMenu->OnAction(executor, action)) return true;

  if (action.wID == ACTION_OSD_SHOW_LEFT)
  {
    if ( m_iCurrentSubMenu > 0)
    {
      m_iCurrentSubMenu--;
    }
    else
    {
      m_iCurrentSubMenu = m_vecSubMenus.size() - 1;
    }
    return true;
  }

  if (action.wID == ACTION_OSD_SHOW_RIGHT)
  {
    if ( m_iCurrentSubMenu + 1 < (int)m_vecSubMenus.size() )
    {
      m_iCurrentSubMenu++;
    }
    else
    {
      m_iCurrentSubMenu = 0;
    }
    return true;
  }

  return false;
}

int COSDMenu::GetX() const
{
  return m_iXPos;
}

void COSDMenu::SetX(int X)
{
  m_iXPos = X;
}

int COSDMenu::GetY() const
{
  return m_iYPos;
}

void COSDMenu::SetY(int Y)
{
  m_iYPos = Y;
}


void COSDMenu::AddSubMenu(const COSDSubMenu& submenu)
{
  m_vecSubMenus.push_back( submenu.Clone() );
}


int COSDMenu::GetSelectedMenu() const
{
  return m_iCurrentSubMenu;
}


void COSDMenu::SetLabel(int iMessage, const CStdString& strLabel)
{
  for (int i = 0; i < (int)m_vecSubMenus.size(); ++i)
  {
    COSDSubMenu* pSubMenu = m_vecSubMenus[i];
    pSubMenu->SetLabel(iMessage, strLabel);
  }
}
void COSDMenu::SetValue(int iMessage, int iValue)
{
  for (int i = 0; i < (int)m_vecSubMenus.size(); ++i)
  {
    COSDSubMenu* pSubMenu = m_vecSubMenus[i];
    pSubMenu->SetValue(iMessage, iValue);
  }
}
