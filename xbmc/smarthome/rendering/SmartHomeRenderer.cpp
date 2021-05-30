/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeRenderer.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/streams/RPStreamManager.h"
#include "smarthome/guibridge/GUIRenderTargetFactory.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/streams/SmartHomeStreamManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeRenderer::CSmartHomeRenderer(CSmartHomeGuiBridge& guiBridge,
                                       CSmartHomeStreamManager& streamManager)
  : m_guiBridge(guiBridge), m_streamManager(streamManager)
{
}

CSmartHomeRenderer::~CSmartHomeRenderer() = default;

void CSmartHomeRenderer::Initialize()
{
  // Initialize process info
  m_processInfo = RETRO::CRPProcessInfo::CreateInstance();
  if (!m_processInfo)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to create - no process info registered");
  }
  else
  {
    // Initialize render manager
    m_renderManager = std::make_unique<RETRO::CRPRenderManager>(*m_processInfo);

    // Initialize stream subsystem
    m_retroStreamManager =
        std::make_unique<RETRO::CRPStreamManager>(*m_renderManager, *m_processInfo);
    m_streamManager.Initialize(*m_retroStreamManager);

    // Initialize GUI
    m_renderTargetFactory = std::make_unique<CGUIRenderTargetFactory>(*m_renderManager);
    m_guiBridge.RegisterRenderer(*m_renderTargetFactory);

    CLog::Log(LOGDEBUG, "SMARTHOME: Created renderer");
  }
}

void CSmartHomeRenderer::Deinitialize()
{
  // Deinitialize GUI
  m_guiBridge.UnregisterRenderer();
  m_renderTargetFactory.reset();

  // Deinitialize stream subsystem
  m_streamManager.Deinitialize();
  m_retroStreamManager.reset();

  // Deinitialize render manager
  m_renderManager.reset();

  // Deinitialize process info
  m_processInfo.reset();
}

void CSmartHomeRenderer::FrameMove()
{
  if (m_renderManager)
    m_renderManager->FrameMove();
}
