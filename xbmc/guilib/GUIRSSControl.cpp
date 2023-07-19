/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRSSControl.h"

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ColorUtils.h"
#include "utils/RssManager.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"

#include <mutex>

using namespace KODI::GUILIB;

CGUIRSSControl::CGUIRSSControl(int parentID,
                               int controlID,
                               float posX,
                               float posY,
                               float width,
                               float height,
                               const CLabelInfo& labelInfo,
                               const GUIINFO::CGUIInfoColor& channelColor,
                               const GUIINFO::CGUIInfoColor& headlineColor,
                               std::string& strRSSTags)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_strRSSTags(strRSSTags),
    m_label(labelInfo),
    m_channelColor(channelColor),
    m_headlineColor(headlineColor),
    m_scrollInfo(0, 0, labelInfo.scrollSpeed, "")
{
  m_pReader = NULL;
  m_rtl = false;
  m_stopped = false;
  m_urlset = 1;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::CGUIRSSControl(const CGUIRSSControl& from)
  : CGUIControl(from),
    m_feed(),
    m_strRSSTags(from.m_strRSSTags),
    m_label(from.m_label),
    m_channelColor(from.m_channelColor),
    m_headlineColor(from.m_headlineColor),
    m_vecUrls(),
    m_vecIntervals(),
    m_scrollInfo(from.m_scrollInfo)
{
  m_pReader = NULL;
  m_rtl = from.m_rtl;
  m_stopped = from.m_stopped;
  m_urlset = 1;
  ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
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

bool CGUIRSSControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
  changed |= m_label.UpdateColors();
  changed |= m_headlineColor.Update();
  changed |= m_channelColor.Update();
  return changed;
}

void CGUIRSSControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool dirty = false;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS) && CRssManager::GetInstance().IsActive())
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
    // Create RSS background/worker thread if needed
    if (m_pReader == NULL)
    {

      RssUrls::const_iterator iter = CRssManager::GetInstance().GetUrls().find(m_urlset);
      if (iter != CRssManager::GetInstance().GetUrls().end())
      {
        m_rtl = iter->second.rtl;
        m_vecUrls = iter->second.url;
        m_vecIntervals = iter->second.interval;
        m_scrollInfo.SetSpeed(m_label.scrollSpeed * (m_rtl ? -1 : 1));
      }

      dirty = true;

      if (CRssManager::GetInstance().GetReader(GetID(), GetParentID(), this, m_pReader))
      {
        m_scrollInfo.m_pixelPos = m_pReader->m_savedScrollPixelPos;
      }
      else
      {
        if (m_strRSSTags != "")
        {
          std::vector<std::string> tags = StringUtils::Split(m_strRSSTags, ",");
          for (const std::string& i : tags)
            m_pReader->AddTag(i);
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
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS) && CRssManager::GetInstance().IsActive())
  {

    if (m_label.font)
    {
      std::vector<UTILS::COLOR::Color> colors;
      colors.push_back(m_label.textColor);
      colors.push_back(m_headlineColor);
      colors.push_back(m_channelColor);
      m_label.font->DrawScrollingText(m_posX, m_posY, colors, m_label.shadowColor, m_feed, 0, m_width, m_scrollInfo);
    }

    if (m_pReader)
    {
      m_pReader->CheckForUpdates();
      m_pReader->m_savedScrollPixelPos = m_scrollInfo.m_pixelPos;
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
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  m_feed = feed;
  m_dirty = true;
}

void CGUIRSSControl::OnFeedRelease()
{
  m_pReader = NULL;
}


