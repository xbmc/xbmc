/*!
\file GUISelectButtonControl.h
\brief
*/

#ifndef GUILIB_GUIWINDOWSELECTCONTROL_H
#define GUILIB_GUIWINDOWSELECTCONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief Button with multi selection choice.

 Behaves like a normal button control, but when pressing,
 it can show multiple strings. The user can choose one by
 moving left or right. \n
 \n
 Messages the button reactes on: \n

 - GUI_MSG_LABEL_ADD \n
 Add a label to the control. Use CGUIMessage::SetLabel
 to set the label text.
 - GUI_MSG_LABEL_RESET \n
 Remove all labels from the control.
 - GUI_MSG_ITEM_SELECTED \n
 After sending this message the CGUIMessage::GetParam1
 contains the selected label as an integer.
 \note The order of the items depends on the order they have been added to
 the control using GUI_MSG_LABEL_ADD.
 - GUI_MSG_ITEM_SELECT \n
 Send this message with CGUIMessage::SetParam1() set to the label
 to be selected. \n
 \n
 Example entry to define a select button in a window or as reference control: \n
 \verbatim
    <control>
      <description>default select button</description
      <type>selectbutton</type>
      <id>6</id>
      <posX>60</posX>
      <posY>192</posY>
      <width>130</width>
      <height>32</height>
      <label>132</label>
      <font>font13</font>
      <textureFocus>button-focus.png</textureFocus>
      <textureNoFocus>button-nofocus.jpg</textureNoFocus>
      <texturebg>button-focus.png</texturebg>
      <textureLeft>scroll-left.png</textureLeft>
      <textureRight>scroll-right.png</textureRight>
      <font>font13</font>
      <textcolor>ffffffff</textcolor>
      <colordiffuse>ffffffff</colordiffuse>
      <disabledcolor>60ffffff</disabledcolor>
      <onleft>50</onleft>
      <onright>50</onright>
      <onup>3</onup>
      <ondown>7</ondown>
    </control>
  \endverbatim

 \sa CGUIMessage
 */
class CGUISelectButtonControl : public CGUIButtonControl
{
public:
  CGUISelectButtonControl(int parentID, int controlID,
                          float posX, float posY,
                          float width, float height,
                          const CTextureInfo& buttonFocus, const CTextureInfo& button,
                          const CLabelInfo& labelInfo,
                          const CTextureInfo& selectBackground,
                          const CTextureInfo& selectArrowLeft, const CTextureInfo& selectArrowLeftFocus,
                          const CTextureInfo& selectArrowRight, const CTextureInfo& selectArrowRightFocus);
  virtual ~CGUISelectButtonControl(void);
  virtual CGUISelectButtonControl *Clone() const { return new CGUISelectButtonControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnMouseOver(const CPoint &point);

  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);

protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual bool UpdateColors();
  bool m_bShowSelect;
  CGUITexture m_imgBackground;
  CGUITexture m_imgLeft;
  CGUITexture m_imgLeftFocus;
  CGUITexture m_imgRight;
  CGUITexture m_imgRightFocus;
  std::vector<std::string> m_vecItems;
  int m_iCurrentItem;
  int m_iDefaultItem;
  int m_iStartFrame;
  bool m_bLeftSelected;
  bool m_bRightSelected;
  bool m_bMovedLeft;
  bool m_bMovedRight;
  unsigned int m_ticks;
};
#endif
