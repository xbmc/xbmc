/*!
\file GUIListLabel.h
\brief 
*/

#pragma once

#include "GUIControl.h"

#include "GUILabelControl.h"  // for CLabelInfo
/*!
 \ingroup controls
 \brief 
 */
class CGUIListLabel :
      public CGUIControl
{
public:
  CGUIListLabel(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo);
  virtual ~CGUIListLabel(void);

  virtual void Render();
  virtual bool CanFocus() const { return false; };

  const CRect &GetRenderRect() const { return m_renderRect; };
  void SetRenderRect(const CRect &rect) { m_renderRect = rect; };
  void SetLabel(const CStdString &label);
  void SetSelected(bool selected);
  void SetScrolling(bool scrolling);

  const CLabelInfo& GetLabelInfo() const { return m_label; };

protected:
  CLabelInfo  m_label;
  CStdStringW m_text;
  float       m_textWidth;

  bool        m_scrolling;
  bool        m_selected;
  CScrollInfo m_scrollInfo;
  CRect       m_renderRect;   // render location
};
