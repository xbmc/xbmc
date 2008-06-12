/*!
\file GUIButtonControl.h
\brief 
*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

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

#include "guiImage.h"
#include "GUILabelControl.h"  // for CInfoPortion

/*!
 \ingroup controls
 \brief 
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId,
                    float posX, float posY, float width, float height,
                    const CImage& textureFocus, const CImage& textureNoFocus,
                    const CLabelInfo &label);

  virtual ~CGUIButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(const CGUIInfoColor &color);
  void SetLabel(const std::string & aLabel);
  void SetLabel2(const std::string & aLabel2);
  void SetClickActions(const std::vector<CStdString>& clickActions) { m_clickActions = clickActions; };
  const std::vector<CStdString> &GetClickActions() const { return m_clickActions; };
  void SetFocusActions(const std::vector<CStdString>& focusActions) { m_focusActions = focusActions; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  virtual CStdString GetLabel() const { return GetDescription(); };
  virtual CStdString GetLabel2() const;
  void SetTabButton(bool bIsTabButton = TRUE) { m_bTabButton = bIsTabButton; };
  void SetSelected(bool bSelected);
  void Flicker(bool bFlicker = TRUE);
  virtual void Update();
  virtual CStdString GetDescription() const;
  void SetAlpha(unsigned char alpha);

  void PythonSetLabel(const CStdString &strFont, const std::string &strText, DWORD dwTextColor, DWORD dwShadowColor, DWORD dwFocusedColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

  void RAMSetTextColor(DWORD dwTextColor);
  void SettingsCategorySetTextAlign(DWORD dwAlign);

  virtual void OnClick();
  bool HasClickActions() { return m_clickActions.size() > 0; };

protected:
  void OnFocus();

  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;
  DWORD m_dwFocusCounter;
  DWORD m_dwFlickerCounter;
  DWORD m_dwFrameCounter;
  unsigned char m_alpha;

  CGUIInfoLabel  m_info;
  CGUIInfoLabel  m_info2;
  CLabelInfo m_label;
  CGUITextLayout m_textLayout;
  CGUITextLayout m_textLayout2;

  std::vector<CStdString> m_clickActions;
  std::vector<CStdString> m_focusActions;
  bool m_bTabButton;

  bool m_bSelected;
};
#endif
