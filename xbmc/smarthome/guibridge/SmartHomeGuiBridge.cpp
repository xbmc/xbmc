/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeGuiBridge.h"

#include "GUIRenderHandle.h"
#include "GUIRenderTarget.h"
#include "GUIRenderTargetFactory.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeGuiBridge::~CSmartHomeGuiBridge() = default;

void CSmartHomeGuiBridge::RegisterRenderer(CGUIRenderTargetFactory& factory)
{
  // Set factory
  {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_factory = &factory;
    UpdateRenderTargets();
  }
}

void CSmartHomeGuiBridge::UnregisterRenderer()
{
  // Reset factory
  {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_factory = nullptr;
    UpdateRenderTargets();
  }
}

std::shared_ptr<CGUIRenderHandle> CSmartHomeGuiBridge::RegisterControl(CGUICameraControl& control)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  // Create handle for camera view control
  std::shared_ptr<CGUIRenderHandle> renderHandle(new CGUIRenderControlHandle(*this, control));

  std::shared_ptr<CGUIRenderTarget> renderTarget;
  if (m_factory != nullptr)
    renderTarget.reset(m_factory->CreateRenderControl(control));

  m_renderTargets.insert(std::make_pair(renderHandle.get(), std::move(renderTarget)));

  return renderHandle;
}

void CSmartHomeGuiBridge::UnregisterHandle(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  m_renderTargets.erase(handle);
}

void CSmartHomeGuiBridge::Render(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->Render();
  }
}

void CSmartHomeGuiBridge::ClearBackground(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->ClearBackground();
  }
}

bool CSmartHomeGuiBridge::IsDirty(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      return renderTarget->IsDirty();
  }

  return false;
}

void CSmartHomeGuiBridge::UpdateRenderTargets()
{
  if (m_factory != nullptr)
  {
    for (auto& it : m_renderTargets)
    {
      CGUIRenderHandle* handle = it.first;
      std::shared_ptr<CGUIRenderTarget>& renderTarget = it.second;

      if (!renderTarget)
        renderTarget.reset(CreateRenderTarget(handle));
    }
  }
  else
  {
    for (auto& it : m_renderTargets)
      it.second.reset();
  }
}

CGUIRenderTarget* CSmartHomeGuiBridge::CreateRenderTarget(CGUIRenderHandle* handle)
{
  CGUIRenderControlHandle* controlHandle = static_cast<CGUIRenderControlHandle*>(handle);
  return m_factory->CreateRenderControl(controlHandle->GetControl());
}
