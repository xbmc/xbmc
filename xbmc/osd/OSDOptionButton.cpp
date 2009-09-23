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
#include "OSDOptionButton.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace OSD;


COSDOptionButton::COSDOptionButton(int iAction, int iHeading)
{
  m_iHeading = iHeading;
  m_iAction = iAction;
  m_strValue = "";
}


COSDOptionButton::COSDOptionButton(const COSDOptionButton& option)
{
  *this = option;
}

const OSD::COSDOptionButton& COSDOptionButton::operator = (const COSDOptionButton& option)
{
  if (this == &option) return * this;
  m_iHeading = option.m_iHeading;
  m_iAction = option.m_iAction;
  m_strValue = option.m_strValue;
  return *this;
}

COSDOptionButton::~COSDOptionButton(void)
{}

IOSDOption* COSDOptionButton::Clone() const
{
  return new COSDOptionButton(*this);
}


void COSDOptionButton::Draw(int x, int y, bool bFocus, bool bSelected)
{
  DWORD dwColor = 0xff999999;
  if (bFocus)
    dwColor = 0xffffffff;
  CGUIFont* pFont13 = g_fontManager.GetFont("font13");
  if (pFont13)
  {
    wstring strHeading = g_localizeStrings.Get(m_iHeading);
    WCHAR wsText[256];
    if (m_strValue.size())
    {
      swprintf(wsText, L"%s %S", strHeading.c_str(), m_strValue.c_str());
    }
    else
    {
      swprintf(wsText, L"%s", strHeading.c_str());
    }
    pFont13->DrawShadowText( (float)x, (float)y, dwColor,
                             wsText, 0,
                             0,
                             2,
                             2,
                             0xFF020202);
  }
}

bool COSDOptionButton::OnAction(IExecutor& executor, const CAction& action)
{
  if (action.wID == ACTION_OSD_SHOW_SELECT)
  {
    executor.OnExecute(m_iAction, this);
    return true;
  }
  return false;
}

void COSDOptionButton::SetLabel(const CStdString& strValue)
{
  m_strValue = strValue;
}
