/*!
\file GUIMoverControl.h
\brief
*/

#ifndef GUILIB_GUIMoverCONTROL_H
#define GUILIB_GUIMoverCONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUITexture.h"
#include "GUIControl.h"

#define ALLOWED_DIRECTIONS_ALL   0
#define ALLOWED_DIRECTIONS_UPDOWN  1
#define ALLOWED_DIRECTIONS_LEFTRIGHT 2

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

/*!
 \ingroup controls
 \brief
 */
class CGUIMoverControl : public CGUIControl
{
public:
  CGUIMoverControl(int parentID, int controlID,
                   float posX, float posY, float width, float height,
                   const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus);

  ~CGUIMoverControl(void) override;
  CGUIMoverControl *Clone() const override { return new CGUIMoverControl(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override;
  void OnUp() override;
  void OnDown() override;
  void OnLeft() override;
  void OnRight() override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;
  void SetLimits(int iX1, int iY1, int iX2, int iY2);
  void SetLocation(int iLocX, int iLocY, bool bSetPosition = true);
  int GetXLocation() const { return m_iLocationX;};
  int GetYLocation() const { return m_iLocationY;};
  bool CanFocus() const override { return true; };

protected:
  EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) override;
  bool UpdateColors() override;
  bool SetAlpha(unsigned char alpha);
  void UpdateSpeed(int nDirection);
  void Move(int iX, int iY);
  CGUITexture m_imgFocus;
  CGUITexture m_imgNoFocus;
  unsigned int m_frameCounter;
  unsigned int m_lastMoveTime;
  int m_nDirection;
  float m_fSpeed;
  float m_fAnalogSpeed;
  float m_fMaxSpeed;
  float m_fAcceleration;
  int m_iX1, m_iX2, m_iY1, m_iY2;
  int m_iLocationX, m_iLocationY;
};
#endif
