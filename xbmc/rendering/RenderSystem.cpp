/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystem.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "windowing/WinSystem.h"

#include <memory>

CRenderSystemBase::CRenderSystemBase()
{
  OnAdvancedSettingsLoaded();

  const auto advSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};

  m_settingsCallbackHandle =
      advSettings->RegisterSettingsLoadedCallback([this]() { OnAdvancedSettingsLoaded(); });
}

void CRenderSystemBase::OnAdvancedSettingsLoaded()
{
  const auto advSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};

  std::unique_lock lock(m_settingsSection);

  m_showSplashImage = advSettings->m_splashImage;
  m_guiFrontToBackRendering = advSettings->m_guiFrontToBackRendering;
  m_guiGeometryClear =
      advSettings->m_guiGeometryClear ? ClearFunction::GEOMETRY : ClearFunction::FIXED_FUNCTION;
}

CRenderSystemBase::~CRenderSystemBase()
{
  const auto advSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};

  if (m_settingsCallbackHandle.has_value() && advSettings != nullptr)
    advSettings->UnregisterSettingsLoadedCallback(m_settingsCallbackHandle.value());
}

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_RenderVersionMajor;
  minor = m_RenderVersionMinor;
}

bool CRenderSystemBase::SupportsNPOT(bool dxt) const
{
  if (dxt)
    return false;

  return true;
}

bool CRenderSystemBase::SupportsStereo(RenderStereoMode mode) const
{
  switch(mode)
  {
    case RenderStereoMode::OFF:
    case RenderStereoMode::SPLIT_HORIZONTAL:
    case RenderStereoMode::SPLIT_VERTICAL:
    case RenderStereoMode::MONO:
      return true;
    default:
      return false;
  }
}

void CRenderSystemBase::ShowSplash(const std::string& message)
{
  if (!GetShowSplashImage() && !(m_splashImage || !message.empty()))
    return;

  if (!m_splashImage)
  {
    m_splashImage = std::make_unique<CGUIImage>(
        0, 0, .0f, .0f,
        static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()),
        static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()),
        CTextureInfo(CUtil::GetSplashPath()));
    m_splashImage->SetAspectRatio(CAspectRatio::SCALE);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0xff000000);

  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(res, true);

  //render splash image
  BeginRender();

  m_splashImage->AllocResources();
  m_splashImage->Render();
  m_splashImage->FreeResources();

  if (!message.empty())
  {
    if (!m_splashMessageLayout)
    {
      auto messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 40,
                                               FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_splashMessageLayout = std::make_unique<CGUITextLayout>(messageFont, true, .0f);
    }

    if (m_splashMessageLayout)
    {
      m_splashMessageLayout->Update(message, 1150, false, true);
      float textWidth, textHeight;
      m_splashMessageLayout->GetTextExtent(textWidth, textHeight);

      int width = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
      int height = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
      float y = height - textHeight - 100;
      m_splashMessageLayout->RenderOutline(width/2, y, 0, 0xFF000000, XBFONT_CENTER_X, width);
    }
  }

  //show it on screen
  EndRender();
  CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(true, false);
}

bool CRenderSystemBase::GetShowSplashImage()
{
  std::unique_lock lock(m_settingsSection);
  return m_showSplashImage;
}

bool CRenderSystemBase::GetEnabledFrontToBackRendering()
{
  std::unique_lock lock(m_settingsSection);
  return m_guiFrontToBackRendering;
}

ClearFunction CRenderSystemBase::GetClearFunction()
{
  std::unique_lock lock(m_settingsSection);
  return m_guiGeometryClear;
}
