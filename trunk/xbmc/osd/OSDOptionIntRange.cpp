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
#include "OSDOptionIntRange.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace OSD;

COSDOptionIntRange::COSDOptionIntRange(int iAction, int iHeading, bool bPercent)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  m_iMin = 0;
  m_iMax = 10;
  m_iValue = 0;
  m_iInterval = 1;
  m_iHeading = iHeading;
  m_iAction = iAction;
  m_bPercent = bPercent;
}

COSDOptionIntRange::COSDOptionIntRange(int iAction, int iHeading, bool bPercent, int iStart, int iEnd, int iInterval, int iValue)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  m_iMin = iStart;
  m_iMax = iEnd;
  m_iValue = iValue;
  m_iInterval = iInterval;
  m_iHeading = iHeading;
  m_iAction = iAction;
  m_bPercent = bPercent;
}

COSDOptionIntRange::COSDOptionIntRange(const COSDOptionIntRange& option)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  *this = option;
}

const OSD::COSDOptionIntRange& COSDOptionIntRange::operator = (const COSDOptionIntRange& option)
{
  if (this == &option) return * this;

  m_iMin = option.m_iMin;
  m_iMax = option.m_iMax;
  m_iValue = option.m_iValue;
  m_iInterval = option.m_iInterval;
  m_iHeading = option.m_iHeading;
  m_iAction = option.m_iAction;
  m_bPercent = option.m_bPercent;
  return *this;
}

COSDOptionIntRange::~COSDOptionIntRange(void)
{}

IOSDOption* COSDOptionIntRange::Clone() const
{
  return new COSDOptionIntRange(*this);
}


void COSDOptionIntRange::Draw(int x, int y, bool bFocus, bool bSelected)
{
  DWORD dwColor = 0xff999999;
  if (bFocus)
    dwColor = 0xffffffff;
  CGUIFont* pFont13 = g_fontManager.GetFont("font13");
  if (pFont13)
  {
    wstring strHeading = g_localizeStrings.Get(m_iHeading);
    pFont13->DrawShadowText( (float)x, (float)y, dwColor,
                             strHeading.c_str(), 0,
                             0,
                             2,
                             2,
                             0xFF020202);
    WCHAR strValue[128];
    if (m_bPercent)
    {
      swprintf(strValue, L"%i%%", m_iValue);
    }
    else
    {
      if (m_iInterval == 1)
        swprintf(strValue, L"%i/%i", m_iValue, m_iMax);
      else
        swprintf(strValue, L"%i/%i", m_iValue, (m_iMax - m_iMin) / m_iInterval);
    }
    pFont13->DrawShadowText( (float)x + 150, (float)y, dwColor,
                             strValue, 0,
                             0,
                             2,
                             2,
                             0xFF020202);
  }

  float fRange = (float)(m_iMax - m_iMin);
  float fPos = (float)(m_iValue - m_iMin);
  float fPercent = (fPos / fRange) * 100.0f;
  m_slider.SetPercentage( (int) fPercent);
  m_slider.AllocResources();
  m_slider.SetPosition(x + 200, y);
  m_slider.Render();
  m_slider.FreeResources();
}

bool COSDOptionIntRange::OnAction(IExecutor& executor, const CAction& action)
{
  if (action.wID == ACTION_OSD_SHOW_VALUE_PLUS)
  {
    if (m_iValue + m_iInterval <= m_iMax)
    {
      m_iValue += m_iInterval ;
      executor.OnExecute(m_iAction, this);
    }
    else
    {
      m_iValue = m_iMin;
      executor.OnExecute(m_iAction, this);
    }
    return true;
  }
  if (action.wID == ACTION_OSD_SHOW_VALUE_MIN)
  {
    if (m_iValue - m_iInterval >= m_iMin)
    {
      m_iValue -= m_iInterval ;
      executor.OnExecute(m_iAction, this);
    }
    else
    {
      m_iValue = m_iMax;
      executor.OnExecute(m_iAction, this);
    }
    return true;
  }
  return false;
}

int COSDOptionIntRange::GetValue() const
{
  return m_iValue;
}
