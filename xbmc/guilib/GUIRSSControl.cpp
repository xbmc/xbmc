/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/RssManager.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"

using namespace std;

CGUIRSSControl::CGUIRSSControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoColor &channelColor, const CGUIInfoColor &headlineColor, std::string& strRSSTags)
: CGUIControl(parentID, controlID, posX, posY, width, height),
  m_strRSSTags(strRSSTags),
  m_label(labelInfo),
  m_channelColor(channelColor),
  m_headlineColor(headlineColor),
  m_scrollInfo(0,0,labelInfo.scrollSpeed,""),
  m_dirty(true)
{
  m_pReader = NULL;
  m_rtl = false;
  m_stopped = false;
  m_urlset = 1;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::CGUIRSSControl(const CGUIRSSControl &from)
  : CGUIControl(from),
  m_feed(),
  m_strRSSTags(from.m_strRSSTags),
  m_label(from.m_label),
  m_channelColor(from.m_channelColor),
  m_headlineColor(from.m_headlineColor),
  m_vecUrls(),
  m_vecIntervals(),
  m_scrollInfo(from.m_scrollInfo),
  m_dirty(true)
{
  m_pReader = NULL;
  m_rtl = from.m_rtl;
  m_stopped = from.m_stopped;
  m_urlset = 1;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  CSingleLock lock(m_criticalSection);
  if (m_pReader)
    m_pReader->SetObserver(NULL);
  m_pReader = NULL;
}

void CGUIRSSControl::OnFocus()
{
  m_stopped = true;
}

void CGUIRSSControl::OnUnFocus()
{
  m_stopped = false;
}

void CGUIRSSControl::SetUrlSet(const int urlset)
{
  m_urlset = urlset;
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
  bool dirty = false;
  if (CSettings::Get().GetBool("lookandfeel.enablerssfeeds") && CRssManager::Get().IsActive())
  {
    CSingleLock lock(m_criticalSection);
    // Create RSS background/worker thread if needed
    if (m_pReader == NULL)
    {

      RssUrls::const_iterator iter = CRssManager::Get().GetUrls().find(m_urlset);
      if (iter != CRssManager::Get().GetUrls().end())
      {
        m_rtl = iter->second.rtl;
        m_vecUrls = iter->second.url;
        m_vecIntervals = iter->second.interval;
        m_scrollInfo.SetSpeed(m_label.scrollSpeed * (m_rtl ? -1 : 1));
      }

      dirty = true;

      if (CRssManager::Get().GetReader(GetID(), GetParentID(), this, m_pReader))
      {
        m_scrollInfo.pixelPos = m_pReader->m_savedScrollPixelPos;
      }
      else
      {
        if (m_strRSSTags != "")
        {
          vector<string> tags = StringUtils::Split(m_strRSSTags, ",");
          for (vector<string>::const_iterator i = tags.begin(); i != tags.end(); ++i)
            m_pReader->AddTag(*i);
        }
        // use half the width of the control as spacing between feeds, and double this between feed sets
        float spaceWidth = (m_label.font) ? m_label.font->GetCharWidth(L' ') : 15;
        m_pReader->Create(this, m_vecUrls, m_vecIntervals, (int)(0.5f*GetWidth() / spaceWidth) + 1, m_rtl);
      }
    }

    if(m_dirty)
      dirty = true;
    m_dirty = false;

    if (m_label.font)
    {
      if ( m_stopped )
        m_scrollInfo.SetSpeed(0);
      else
        m_scrollInfo.SetSpeed(m_label.scrollSpeed * (m_rtl ? -1 : 1));

      if(m_label.font->UpdateScrollInfo(m_feed, m_scrollInfo))
        dirty = true;
    }
  }

  if(dirty)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIRSSControl::Render()
{
  // only render the control if they are enabled
  if (CSettings::Get().GetBool("lookandfeel.enablerssfeeds") && CRssManager::Get().IsActive())
  {

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
      m_pReader->m_savedScrollPixelPos = m_scrollInfo.pixelPos;
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
  m_dirty = true;
}

void CGUIRSSControl::OnFeedRelease()
{
  m_pReader = NULL;
}


