/*!
\file GUIRSSControl.h
\brief 
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

#pragma once

#include "GUIControl.h"
#include "../xbmc/utils/RssReader.h"

/*!
\ingroup controls
\brief 
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoColor &channelColor, const CGUIInfoColor &headlineColor, CStdString& strRSSTags);
  virtual ~CGUIRSSControl(void);

  virtual void Render();
  virtual void OnFeedUpdate(const std::vector<DWORD> &feed);
  virtual bool CanFocus() const { return false; };

  void SetIntervals(const std::vector<int>& vecIntervals);
  void SetUrls(const std::vector<std::string>& vecUrl);

protected:

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  std::vector<DWORD> m_feed;

  CStdString m_strRSSTags;

  CLabelInfo m_label;
  CGUIInfoColor m_channelColor;
  CGUIInfoColor m_headlineColor;

  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  CScrollInfo m_scrollInfo;
};
#endif
