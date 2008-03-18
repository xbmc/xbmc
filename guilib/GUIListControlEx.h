/*!
\file GUIListControlEx.h
\brief 
*/

#ifndef GUILIB_GUIListControlEx_H
#define GUILIB_GUIListControlEx_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListExItem.h"
#include "GUIList.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIListControlEx : public CGUIControl
{
public:
  CGUIListControlEx(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                    float spinWidth, float spinHeight,
                    const CImage& textureUp, const CImage& textureDown,
                    const CImage& textureUpFocus, const CImage& textureDownFocus,
                    const CLabelInfo& spinInfo, float spinX, float spinY,
                    const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                    const CImage& button, const CImage& buttonFocus);

  virtual ~CGUIListControlEx(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPulseOnSelect(bool pulse);
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);

  void SetScrollySuffix(const CStdString& wstrSuffix);
  void SetImageDimensions(float width, float height);
  void SetItemHeight(float height);
  void SetSpaceBetweenItems(float spaceBetweenItems);
  void SetPageControlVisible(bool bVisible);
  virtual CStdString GetDescription() const;

protected:

  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  void OnPageUp();
  void OnPageDown();
  int GetPage(int listSize);

  float m_spinPosX;
  float m_spinPosY;
  float m_spaceBetweenItems;
  float m_imageWidth;
  float m_imageHeight;
  float m_itemHeight;
  float m_smoothScrollOffset;
  int m_iOffset;
  int m_iItemsPerPage;
  int m_iSelect;
  int m_iCursorY;
  bool m_bUpDownVisible;
  CLabelInfo m_label;
  CLabelInfo m_label2;

  CGUISpinControl m_upDown;
  CGUIButtonControl m_imgButton;
  std::string m_strSuffix;
  CGUIList* m_pList;
};
#endif
