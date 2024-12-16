/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace SMART_HOME
{
class CGUICameraControl;
class CSmartHomeGuiBridge;

// --- CGUIRenderHandle ------------------------------------------------------

class CGUIRenderHandle
{
public:
  CGUIRenderHandle(CSmartHomeGuiBridge& renderManager);
  virtual ~CGUIRenderHandle();

  void Render();
  bool IsDirty();
  void ClearBackground();

private:
  // Construction parameters
  CSmartHomeGuiBridge& m_renderManager;
};

// --- CGUIRenderControlHandle -----------------------------------------------

class CGUIRenderControlHandle : public CGUIRenderHandle
{
public:
  CGUIRenderControlHandle(CSmartHomeGuiBridge& renderManager, CGUICameraControl& control);
  ~CGUIRenderControlHandle() override = default;

  CGUICameraControl& GetControl() { return m_control; }

private:
  // Construction parameters
  CGUICameraControl& m_control;
};

} // namespace SMART_HOME
} // namespace KODI
