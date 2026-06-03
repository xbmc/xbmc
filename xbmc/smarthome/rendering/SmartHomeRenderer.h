/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;
class CRPRenderManager;
class CRPStreamManager;
} // namespace RETRO

namespace SMART_HOME
{
class CGUIRenderTargetFactory;
class CSmartHomeGuiBridge;
class CSmartHomeStreamManager;

class CSmartHomeRenderer
{
public:
  CSmartHomeRenderer(CSmartHomeGuiBridge& guiBridge, CSmartHomeStreamManager& streamManager);
  ~CSmartHomeRenderer();

  void Initialize();
  void Deinitialize();

  // GUI functions
  void FrameMove();

private:
  // Subsystems
  CSmartHomeGuiBridge& m_guiBridge;
  CSmartHomeStreamManager& m_streamManager;

  // GUI parameters
  std::unique_ptr<CGUIRenderTargetFactory> m_renderTargetFactory;

  // RetroPlayer parameters
  std::unique_ptr<RETRO::CRPProcessInfo> m_processInfo;
  std::unique_ptr<RETRO::CRPRenderManager> m_renderManager;
  std::unique_ptr<RETRO::CRPStreamManager> m_retroStreamManager;
};

} // namespace SMART_HOME
} // namespace KODI
