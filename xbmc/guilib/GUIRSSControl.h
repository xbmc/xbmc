/*!
\file GUIRSSControl.h
\brief
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

#pragma once

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

#include "GUIControl.h"
#include "GUILabel.h"
#include "utils/IRssObserver.h"

typedef uint32_t color_t;
typedef std::vector<color_t> vecColors;

class CRssReader;

/*!
\ingroup controls
\brief
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoColor &channelColor, const CGUIInfoColor &headlineColor, std::string& strRSSTags);
  CGUIRSSControl(const CGUIRSSControl &from);
  virtual ~CGUIRSSControl(void);
  virtual CGUIRSSControl *Clone() const { return new CGUIRSSControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void OnFeedUpdate(const vecText &feed);
  virtual void OnFeedRelease();
  virtual bool CanFocus() const { return true; };
  virtual CRect CalcRenderRegion() const;

  virtual void OnFocus();
  virtual void OnUnFocus();

  void SetUrlSet(const int urlset);

protected:
  virtual bool UpdateColors();

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  vecText m_feed;

  std::string m_strRSSTags;

  CLabelInfo m_label;
  CGUIInfoColor m_channelColor;
  CGUIInfoColor m_headlineColor;

  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  bool m_rtl;
  CScrollInfo m_scrollInfo;
  bool m_dirty;
  bool m_stopped;
  int  m_urlset;
};
#endif
