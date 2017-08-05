/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "guilib/GUIControl.h"
#include "guilib/GUIInfoTypes.h"

class CGUIInfoLabel;

namespace KODI
{
namespace RETRO
{

class CGUIGameControl : public CGUIControl
{
public:
  CGUIGameControl(int parentID, int controlID, float posX, float posY, float width, float height);
  ~CGUIGameControl() override = default;

  void SetViewMode(const CGUIInfoLabel &viewMode);
  void SetVideoFilter(const CGUIInfoLabel &videoFilter);

  // implementation of CGUIControl
  CGUIGameControl *Clone() const override { return new CGUIGameControl(*this); };
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void RenderEx() override;
  bool CanFocus() const override;
  void UpdateInfo(const CGUIListItem *item = nullptr) override;

private:
  void EnableGUIRender();
  void DisableGUIRender();

  CGUIInfoLabel m_viewModeInfo;
  CGUIInfoLabel m_videoFilterInfo;

  int m_viewMode = -1;
  int m_scalingMethod = -1;
};

}
}
