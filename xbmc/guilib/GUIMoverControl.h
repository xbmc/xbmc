/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMoverControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"
#include "utils/MovingSpeed.h"

#define ALLOWED_DIRECTIONS_ALL   0
#define ALLOWED_DIRECTIONS_UPDOWN  1
#define ALLOWED_DIRECTIONS_LEFTRIGHT 2

/*!
 \ingroup controls
 \brief
 */
class CGUIMoverControl : public CGUIControl
{
public:
  CGUIMoverControl(int parentID,
                   int controlID,
                   float posX,
                   float posY,
                   float width,
                   float height,
                   const CTextureInfo& textureFocus,
                   const CTextureInfo& textureNoFocus,
                   UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg);

  ~CGUIMoverControl(void) override = default;
  CGUIMoverControl* Clone() const override { return new CGUIMoverControl(*this); }

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
  int GetXLocation() const { return m_iLocationX; }
  int GetYLocation() const { return m_iLocationY; }
  bool CanFocus() const override { return true; }

protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool UpdateColors(const CGUIListItem* item) override;
  bool SetAlpha(unsigned char alpha);
  void Move(int iX, int iY);
  std::unique_ptr<CGUITexture> m_imgFocus;
  std::unique_ptr<CGUITexture> m_imgNoFocus;
  unsigned int m_frameCounter;
  UTILS::MOVING_SPEED::CMovingSpeed m_movingSpeed;
  float m_fAnalogSpeed;
  int m_iX1, m_iX2, m_iY1, m_iY2;
  int m_iLocationX, m_iLocationY;

private:
  CGUIMoverControl(const CGUIMoverControl& control);
};

