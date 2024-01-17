/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRESIZEControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"
#include "utils/MovingSpeed.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIResizeControl : public CGUIControl
{
public:
  CGUIResizeControl(int parentID,
                    int controlID,
                    float posX,
                    float posY,
                    float width,
                    float height,
                    const CTextureInfo& textureFocus,
                    const CTextureInfo& textureNoFocus,
                    UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg);

  ~CGUIResizeControl() override = default;
  CGUIResizeControl* Clone() const override { return new CGUIResizeControl(*this); }

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
  void SetLimits(float x1, float y1, float x2, float y2);
  bool CanFocus() const override { return true; }

protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool UpdateColors(const CGUIListItem* item) override;
  bool SetAlpha(unsigned char alpha);
  void Resize(float x, float y);
  std::unique_ptr<CGUITexture> m_imgFocus;
  std::unique_ptr<CGUITexture> m_imgNoFocus;
  unsigned int m_frameCounter;
  UTILS::MOVING_SPEED::CMovingSpeed m_movingSpeed;
  float m_fAnalogSpeed;
  float m_x1, m_x2, m_y1, m_y2;

private:
  CGUIResizeControl(const CGUIResizeControl& control);
};

