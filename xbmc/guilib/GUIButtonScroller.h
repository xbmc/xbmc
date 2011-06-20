#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIControl.h"
#include "GUITexture.h"
#include "GUIActionDescriptor.h"
#include "GUILabel.h"

class TiXmlNode;

class CButton
{
public:
  CButton()
  {
    id = 0;
    info = 0;
    imageFocus = imageNoFocus = NULL;
  };
  ~CButton()
  {
    delete imageFocus;
    delete imageNoFocus;
  }
  int id;
  int info;
  std::string strLabel;
  std::vector<CGUIActionDescriptor> clickActions;
  CGUITexture *imageFocus;
  CGUITexture *imageNoFocus;
};

class CGUIButtonScroller :
      public CGUIControl
{
public:
  CGUIButtonScroller(int parentID, int controlID, float posX, float posY, float width, float height, float gap, int iSlots, int iDefaultSlot, int iMovementRange, bool bHorizontal, int iAlpha, bool bWrapAround, bool bSmoothScrolling, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CLabelInfo& labelInfo);
  CGUIButtonScroller(const CGUIButtonScroller &from);
  virtual ~CGUIButtonScroller(void);
  virtual CGUIButtonScroller *Clone() const { return new CGUIButtonScroller(*this); };

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnDown();
  virtual bool OnMouseOver(const CPoint &point);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  void ClearButtons();
  void AddButton(const std::string &strLabel, const CStdString &strExecute, const int iIcon);
  void SetActiveButton(int iButton);
  int GetActiveButton() const;
  int GetActiveButtonID() const;
  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  void LoadButtons(TiXmlNode *node);

protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual bool UpdateColors();
  int GetNext(int iCurrent) const;
  int GetPrevious(int iCurrent);
  int GetButton(int iOffset);
  void DoUp();
  void DoDown();
  void RenderItem(float &posX, float &posY, int &iOffset, bool bText);
  void GetScrollZone(float &fStartAlpha, float &fEndAlpha);
private:
  // saved variables from the xml (as this control is user editable...)
  int m_iXMLNumSlots;
  int m_iXMLDefaultSlot;
  float m_xmlPosX;
  float m_xmlPosY;
  float m_xmlWidth;
  float m_xmlHeight;

  float m_buttonGap;     // gap between buttons
  int m_iNumSlots;     // number of button slots available
  int m_iDefaultSlot;    // default highlight position
  int m_iMovementRange;   // amoung that we can move the highlight
  bool m_bHorizontal;    // true if it's a horizontal button bar
  int m_iAlpha;       // amount of alpha (0..100)
  bool m_bWrapAround;    // whether the buttons wrap around or not
  bool m_bSmoothScrolling; // whether there is smooth scrolling or not

  int m_iCurrentSlot;    // currently highlighted slot

  int m_iOffset;      // first item in the list
  float m_scrollOffset;   // offset when scrolling
  bool m_bScrollUp;     // true if we're scrolling up (or left)
  bool m_bScrollDown;    // true if scrolling down (or right)
  bool m_bMoveUp;      // true if we're scrolling up (or left)
  bool m_bMoveDown;     // true if scrolling down (or right)
  float m_fScrollSpeed;   // speed of scrolling
  float m_fAnalogScrollSpeed;  // speed of analog scroll (triggers)
  // stuff we need for the buttons...
  std::vector<CButton*> m_vecButtons;
  typedef std::vector<CButton*>::iterator ivecButtons;
  CGUITexture m_imgFocus;
  CGUITexture m_imgNoFocus;

  CLabelInfo m_label;
  int m_iSlowScrollCount;
};
