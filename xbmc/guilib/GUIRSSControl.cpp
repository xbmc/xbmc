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

#include "GUIRSSControl.h"
#include "GUIWindowManager.h"
#include "settings/GUISettings.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"

using namespace std;

CGUIRSSControl::CGUIRSSControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoColor &channelColor, const CGUIInfoColor &headlineColor, CStdString& strRSSTags)
: CGUIControl(parentID, controlID, posX, posY, width, height),
  m_scrollInfo(0,0,labelInfo.scrollSpeed,"")
{
  m_label = labelInfo;
  m_headlineColor = headlineColor;
  m_channelColor = channelColor;

  m_strRSSTags = strRSSTags;

  m_pReader = NULL;
  m_rtl = false;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::CGUIRSSControl(const CGUIRSSControl &from)
: CGUIControl(from),m_scrollInfo(from.m_scrollInfo)
{
  m_label = from.m_label;
  m_headlineColor = from.m_headlineColor;
  m_channelColor = from.m_channelColor;
  m_strRSSTags = from.m_strRSSTags;
  m_pReader = NULL;
  m_rtl = from.m_rtl;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  CSingleLock lock(m_criticalSection);
  if (m_pReader)
    m_pReader->SetObserver(NULL);
  m_pReader = NULL;
}

void CGUIRSSControl::SetUrls(const vector<string> &vecUrl, bool rtl)
{
  m_vecUrls = vecUrl;
  m_rtl = rtl;
  if (m_scrollInfo.pixelSpeed > 0 && rtl)
    m_scrollInfo.pixelSpeed *= -1;
  else if (m_scrollInfo.pixelSpeed < 0 && !rtl)
    m_scrollInfo.pixelSpeed *= -1;
}

void CGUIRSSControl::SetIntervals(const vector<int>& vecIntervals)
{
  m_vecIntervals = vecIntervals;
}

bool CGUIRSSControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();
  changed |= m_headlineColor.Update();
  changed |= m_channelColor.Update();

  return changed;
}

void CGUIRSSControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // TODO Proper processing which marks when its actually changed. Just mark always for now.
  MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIRSSControl::Render()
{
  // only render the control if they are enabled
  if (g_guiSettings.GetBool("lookandfeel.enablerssfeeds") && g_rssManager.IsActive())
  {
    CSingleLock lock(m_criticalSection);
    // Create RSS background/worker thread if needed
    if (m_pReader == NULL)
    {
      if (g_rssManager.GetReader(GetID(), GetParentID(), this, m_pReader))
        m_scrollInfo.characterPos = m_pReader->m_SavedScrollPos;
      else
      {
        if (m_strRSSTags != "")
        {
          CStdStringArray vecSplitTags;

          StringUtils::SplitString(m_strRSSTags, ",", vecSplitTags);

          for (unsigned int i = 0;i < vecSplitTags.size();i++)
            m_pReader->AddTag(vecSplitTags[i]);
        }
        // use half the width of the control as spacing between feeds, and double this between feed sets
        float spaceWidth = (m_label.font) ? m_label.font->GetCharWidth(L' ') : 15;
        m_pReader->Create(this, m_vecUrls, m_vecIntervals, (int)(0.5f*GetWidth() / spaceWidth) + 1, m_rtl);
      }
    }

    if (m_label.font)
    {
      vecColors colors;
      colors.push_back(m_label.textColor);
      colors.push_back(m_headlineColor);
      colors.push_back(m_channelColor);
      m_label.font->DrawScrollingText(m_posX, m_posY, colors, m_label.shadowColor, m_feed, 0, m_width, m_scrollInfo);
    }

    if (m_pReader)
    {
      m_pReader->CheckForUpdates();
      m_pReader->m_SavedScrollPos = m_scrollInfo.characterPos;
    }
  }
  CGUIControl::Render();
}

CRect CGUIRSSControl::CalcRenderRegion() const
{
  if (m_label.font)
    return CRect(m_posX, m_posY, m_posX + m_width, m_posY + m_label.font->GetTextHeight(1));
  return CGUIControl::CalcRenderRegion();
}

void CGUIRSSControl::OnFeedUpdate(const vecText &feed)
{
  CSingleLock lock(m_criticalSection);
  m_feed = feed;
}

void CGUIRSSControl::OnFeedRelease()
{
  m_pReader = NULL;
}


