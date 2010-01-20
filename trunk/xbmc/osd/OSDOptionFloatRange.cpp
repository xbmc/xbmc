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
#include "OSDOptionFloatRange.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace OSD;

COSDOptionFloatRange::COSDOptionFloatRange(int iAction, int iHeading)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  m_fMin = 0.0f;
  m_fMax = 1.0f;
  m_fValue = 0.0f;
  m_fInterval = 0.1f;
  m_iHeading = iHeading;
  m_iAction = iAction;
}

COSDOptionFloatRange::COSDOptionFloatRange(int iAction, int iHeading, float fStart, float fEnd, float fInterval, float fValue)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  m_fMin = fStart;
  m_fMax = fEnd;
  m_fValue = fValue;
  m_fInterval = fInterval;
  m_iHeading = iHeading;
  m_iAction = iAction;
}

COSDOptionFloatRange::COSDOptionFloatRange(const COSDOptionFloatRange& option)
    : m_slider(0, 1, 0, 0, 0, 0, "osd-pnl-bar.bmp", "xb-ctl-nibv.bmp", "xb-ctl-nibv.bmp", 0)
{
  *this = option;
}

const OSD::COSDOptionFloatRange& COSDOptionFloatRange::operator = (const COSDOptionFloatRange& option)
{
  if (this == &option) return * this;

  m_fMin = option.m_fMin;
  m_fMax = option.m_fMax;
  m_fValue = option.m_fValue;
  m_fInterval = option.m_fInterval;
  m_iHeading = option.m_iHeading;
  m_iAction = option.m_iAction;
  return *this;
}

COSDOptionFloatRange::~COSDOptionFloatRange(void)
{}

IOSDOption* COSDOptionFloatRange::Clone() const
{
  return new COSDOptionFloatRange(*this);
}


void COSDOptionFloatRange::Draw(int x, int y, bool bFocus, bool bSelected)
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
    swprintf(strValue, L"%2.2f", m_fValue);
    pFont13->DrawShadowText( (float)x + 150, (float)y, dwColor,
                             strValue, 0,
                             0,
                             2,
                             2,
                             0xFF020202);
  }
  float fRange = (float)(m_fMax - m_fMin);
  float fPos = (float)(m_fValue - m_fMin);
  float fPercent = (fPos / fRange) * 100.0f;
  m_slider.SetPercentage( (int) fPercent);
  m_slider.AllocResources();
  m_slider.SetPosition(x + 200, y);
  m_slider.Render();
  m_slider.FreeResources();
}

bool COSDOptionFloatRange::OnAction(IExecutor& executor, const CAction& action)
{
  if (action.wID == ACTION_OSD_SHOW_VALUE_PLUS)
  {
    if (m_fValue + m_fInterval <= m_fMax)
    {
      m_fValue += m_fInterval ;
    }
    else
    {
      m_fValue = m_fMin;
    }
    executor.OnExecute(m_iAction, this);
    return true;
  }
  if (action.wID == ACTION_OSD_SHOW_VALUE_MIN)
  {
    if (m_fValue - m_fInterval >= m_fMin)
    {
      m_fValue -= m_fInterval ;
    }
    else
    {
      m_fValue = m_fMax;
    }
    executor.OnExecute(m_iAction, this);
    return true;
  }
  return false;
}

float COSDOptionFloatRange::GetValue() const
{
  return m_fValue;
}
