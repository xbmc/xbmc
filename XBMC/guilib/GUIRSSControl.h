/*!
\file GUIRSSControl.h
\brief 
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

#pragma once

#include "GUIControl.h"
#include "..\XBMC\Utils\RssReader.h"

/*!
\ingroup controls
\brief 
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CLabelInfo& labelInfo, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, CStdString& strRSSTags);
  virtual ~CGUIRSSControl(void);

  virtual void Render();
  virtual void OnFeedUpdate(CStdStringW& aFeed, LPBYTE aColorArray);

  DWORD GetChannelTextColor() const { return m_dwChannelColor;};
  DWORD GetHeadlineTextColor() const { return m_dwHeadlineColor;};
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  void SetIntervals(const std::vector<int>& vecIntervals);
  void SetUrls(const std::vector<wstring>& vecUrl);
  const std::vector<wstring>& GetUrls() const { return m_vecUrls; };
  const CStdString& GetTags() const { return m_strRSSTags; };

protected:

  void RenderText();

  CRssReader* m_pReader;
  WCHAR* m_pwzText;
  LPBYTE m_pbColors;

  CStdString m_strRSSTags;
  D3DCOLOR m_dwChannelColor;
  D3DCOLOR m_dwHeadlineColor;

  CLabelInfo m_label;

  int m_iLeadingSpaces;
  std::vector<wstring> m_vecUrls;
  std::vector<int> m_vecIntervals;
  CScrollInfo m_scrollInfo;
};
#endif
