/*!
\file GUISliderControl.h
\brief 
*/

#ifndef GUILIB_GUISLIDERCONTROL_H
#define GUILIB_GUISLIDERCONTROL_H

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

#define SPIN_CONTROL_TYPE_INT       1
#define SPIN_CONTROL_TYPE_FLOAT     2
#define SPIN_CONTROL_TYPE_TEXT      3
#define SPIN_CONTROL_TYPE_FLOAT_ABS 4

/*!
 \ingroup controls
 \brief 
 */
class CGUISliderControl :
      public CGUIControl
{
public:
  CGUISliderControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& backGroundTexture, const CTextureInfo& mibTexture, const CTextureInfo& nibTextureFocus, int iType);
  virtual ~CGUISliderControl(void);
  virtual CGUISliderControl *Clone() const { return new CGUISliderControl(*this); };

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetRange(int iStart, int iEnd);
  virtual void SetFloatRange(float fStart, float fEnd);
  virtual bool OnMessage(CGUIMessage& message);
  void SetInfo(int iInfo);
  void SetPercentage(int iPercent);
  int GetPercentage() const;
  void SetIntValue(int iValue);
  int GetIntValue() const;
  void SetFloatValue(float fValue);
  float GetFloatValue() const;
  void SetFloatInterval(float fInterval);
  void SetType(int iType) { m_iType = iType; };
  void SetControlOffsetX(float controlOffsetX) { m_controlOffsetX = controlOffsetX;};
  void SetControlOffsetY(float controlOffsetY) { m_controlOffsetY = controlOffsetY;};
  virtual bool HitTest(const CPoint &point) const;
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseDrag(const CPoint &offset, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual CStdString GetDescription() const;
  void SetFormatString(const char *format) { if (format) m_formatString = format; };
protected:
  virtual void UpdateColors();
  virtual void Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point);

  CGUITexture m_guiBackground;
  CGUITexture m_guiMid;
  CGUITexture m_guiMidFocus;
  int m_iPercent;
  int m_iType;
  int m_iStart;
  int m_iEnd;
  float m_fStart;
  float m_fEnd;
  int m_iValue;
  float m_fValue;
  float m_fInterval;
  float m_controlOffsetX;
  float m_controlOffsetY;
  int m_iInfoCode;
  bool m_renderText;
  CStdString m_formatString;
};
#endif
