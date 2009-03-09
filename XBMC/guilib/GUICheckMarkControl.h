/*!
\file GUICheckMarkControl.h
\brief
*/

#ifndef CGUILIB_GUICHECKMARK_CONTROL_H
#define CGUILIB_GUICHECKMARK_CONTROL_H

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

#include "GUITexture.h"
#include "GUITextLayout.h"
#include "GUIControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUICheckMarkControl: public CGUIControl
{
public:
  CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& textureCheckMark, const CTextureInfo& textureCheckMarkNF, float checkWidth, float checkHeight, const CLabelInfo &labelInfo);
  virtual ~CGUICheckMarkControl(void);
  virtual CGUICheckMarkControl *Clone() const { return new CGUICheckMarkControl(*this); };

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);

  void SetLabel(const std::string& strLabel);
  const std::string GetLabel() const { return m_strLabel; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  void SetSelected(bool bOnOff);
  bool GetSelected() const;
  bool OnMouseClick(DWORD dwButton, const CPoint &point);

  void PythonSetLabel(const CStdString &strFont, const std::string &strText, DWORD dwTextColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

protected:
  virtual void UpdateColors();
  CGUITexture m_imgCheckMark;
  CGUITexture m_imgCheckMarkNoFocus;

  CLabelInfo m_label;
  CGUITextLayout m_textLayout;
  std::string m_strLabel;
  bool m_bSelected;
};
#endif
