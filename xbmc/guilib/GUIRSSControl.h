/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

/*!
\file GUIRSSControl.h
\brief
*/

#include <vector>

#include "GUIControl.h"
#include "GUILabel.h"
#include "utils/IRssObserver.h"

class CRssReader;

/*!
\ingroup controls
\brief
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(int parentID, int controlID, float posX, float posY, float width, float height,
                 const CLabelInfo& labelInfo, const KODI::GUILIB::GUIINFO::CGUIInfoColor &channelColor,
                 const KODI::GUILIB::GUIINFO::CGUIInfoColor &headlineColor, std::string& strRSSTags);
  CGUIRSSControl(const CGUIRSSControl &from);
  ~CGUIRSSControl(void) override;
  CGUIRSSControl *Clone() const override { return new CGUIRSSControl(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void OnFeedUpdate(const vecText &feed) override;
  void OnFeedRelease() override;
  bool CanFocus() const override { return true; };
  CRect CalcRenderRegion() const override;

  void OnFocus() override;
  void OnUnFocus() override;

  void SetUrlSet(const int urlset);

protected:
  bool UpdateColors() override;

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  vecText m_feed;

  std::string m_strRSSTags;

  CLabelInfo m_label;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_channelColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_headlineColor;

  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  bool m_rtl;
  CScrollInfo m_scrollInfo;
  bool m_dirty;
  bool m_stopped;
  int  m_urlset;
};

