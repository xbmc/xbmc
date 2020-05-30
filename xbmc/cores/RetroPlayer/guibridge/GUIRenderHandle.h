/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{
class CGameWindowFullScreen;
class CGUIGameControl;
class CGUIGameRenderManager;

enum class RENDER_HANDLE
{
  CONTROL,
  WINDOW,
};

// --- CGUIRenderHandle ------------------------------------------------------

class CGUIRenderHandle
{
public:
  CGUIRenderHandle(CGUIGameRenderManager& renderManager, RENDER_HANDLE type);
  virtual ~CGUIRenderHandle();

  RENDER_HANDLE Type() const { return m_type; }

  void Render();
  void RenderEx();
  bool IsDirty();
  void ClearBackground();

private:
  // Construction parameters
  CGUIGameRenderManager& m_renderManager;
  const RENDER_HANDLE m_type;
};

// --- CGUIRenderControlHandle -----------------------------------------------

class CGUIRenderControlHandle : public CGUIRenderHandle
{
public:
  CGUIRenderControlHandle(CGUIGameRenderManager& renderManager, CGUIGameControl& control);
  ~CGUIRenderControlHandle() override = default;

  CGUIGameControl& GetControl() { return m_control; }

private:
  // Construction parameters
  CGUIGameControl& m_control;
};

// --- CGUIRenderFullScreenHandle --------------------------------------------

class CGUIRenderFullScreenHandle : public CGUIRenderHandle
{
public:
  CGUIRenderFullScreenHandle(CGUIGameRenderManager& renderManager, CGameWindowFullScreen& window);
  ~CGUIRenderFullScreenHandle() override = default;

  CGameWindowFullScreen& GetWindow() { return m_window; }

private:
  // Construction parameters
  CGameWindowFullScreen& m_window;
};
} // namespace RETRO
} // namespace KODI
