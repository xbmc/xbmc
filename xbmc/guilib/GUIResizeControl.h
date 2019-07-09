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

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

/*!
 \ingroup controls
 \brief
 */
class CGUIResizeControl : public CGUIControl
{
public:
  CGUIResizeControl(int parentID, int controlID,
                    float posX, float posY, float width, float height,
                    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus);

  ~CGUIResizeControl(void) override;
  CGUIResizeControl *Clone() const override { return new CGUIResizeControl(*this); };

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
  bool CanFocus() const override { return true; };

protected:
  EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) override;
  bool UpdateColors() override;
  bool SetAlpha(unsigned char alpha);
  void UpdateSpeed(int nDirection);
  void Resize(float x, float y);
  CGUITexture m_imgFocus;
  CGUITexture m_imgNoFocus;
  unsigned int m_frameCounter;
  unsigned int m_lastMoveTime;
  int m_nDirection;
  float m_fSpeed;
  float m_fAnalogSpeed;
  float m_fMaxSpeed;
  float m_fAcceleration;
  float m_x1, m_x2, m_y1, m_y2;
};

