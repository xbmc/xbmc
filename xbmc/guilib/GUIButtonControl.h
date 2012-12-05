/*!
\file GUIButtonControl.h
\brief
*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

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

#include "GUITexture.h"
#include "GUILabel.h"
#include "GUIControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(int parentID, int controlID,
                    float posX, float posY, float width, float height,
                    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                    const CLabelInfo &label);

  virtual ~CGUIButtonControl(void);
  virtual CGUIButtonControl *Clone() const { return new CGUIButtonControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetLabel(const std::string & aLabel);
  virtual void SetLabel2(const std::string & aLabel2);
  void SetClickActions(const CGUIAction& clickActions) { m_clickActions = clickActions; };
  const CGUIAction &GetClickActions() const { return m_clickActions; };
  void SetFocusActions(const CGUIAction& focusActions) { m_focusActions = focusActions; };
  void SetUnFocusActions(const CGUIAction& unfocusActions) { m_unfocusActions = unfocusActions; };
  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); };
  virtual CStdString GetLabel() const { return GetDescription(); };
  virtual CStdString GetLabel2() const;
  void SetSelected(bool bSelected);
  virtual CStdString GetDescription() const;
  void SetAlpha(unsigned char alpha);

  void PythonSetLabel(const CStdString &strFont, const std::string &strText, color_t textColor, color_t shadowColor, color_t focusedColor);
  void PythonSetDisabledColor(color_t disabledColor);

  virtual void OnClick();
  bool HasClickActions() { return m_clickActions.HasActionsMeetingCondition(); };

  virtual bool UpdateColors();

  virtual CRect CalcRenderRegion() const;

protected:
  friend class CGUISpinControlEx;
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  void OnFocus();
  void OnUnFocus();
  virtual void ProcessText(unsigned int currentTime);
  virtual void RenderText();
  CGUILabel::COLOR GetTextColor() const;

  CGUITexture m_imgFocus;
  CGUITexture m_imgNoFocus;
  unsigned int  m_focusCounter;
  unsigned char m_alpha;

  CGUIInfoLabel  m_info;
  CGUIInfoLabel  m_info2;
  CGUILabel      m_label;
  CGUILabel      m_label2;

  CGUIAction m_clickActions;
  CGUIAction m_focusActions;
  CGUIAction m_unfocusActions;

  bool m_bSelected;
};
#endif
