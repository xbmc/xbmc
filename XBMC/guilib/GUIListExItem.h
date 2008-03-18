/*!
\file GUIListExItem.h
\brief 
*/

#ifndef GUILIB_GUIListExItem_H
#define GUILIB_GUIListExItem_H

#pragma once
#include "GUIButtonControl.h"
#include "GUIItem.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIListExItem : public CGUIItem
{
public:
class RenderContext : public CGUIItem::RenderContext
  {
  public:
    RenderContext()
    {
      m_pButton = NULL;
      m_bActive = FALSE;
    };
    virtual ~RenderContext(){};

    CGUIButtonControl* m_pButton;
    CLabelInfo m_label;
    bool m_bActive;
  };

  CGUIListExItem(CStdString& aItemName);
  virtual ~CGUIListExItem(void);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void OnPaint(CGUIItem::RenderContext* pContext);
  void SetIcon(CGUIImage* pImage);
  void SetIcon(float width, float height, const CStdString& aTexture);
  DWORD GetFramesFocused() { return m_dwFocusedDuration; };

protected:
  CGUIImage* m_pIcon;
  DWORD m_dwFocusedDuration;
  DWORD m_dwFrameCounter;
};
#endif
