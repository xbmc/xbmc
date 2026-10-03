/*
 *  Copyright (C) 2017-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererHQ.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <cmath>

bool CRendererHQ::Supports(ESCALINGMETHOD method) const
{
  if (method == VS_SCALINGMETHOD_AUTO)
    return true;

  if (DX::DeviceResources::Get()->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_9_3 && !m_renderOrientation)
  {
    if (method == VS_SCALINGMETHOD_CUBIC_MITCHELL ||
        method == VS_SCALINGMETHOD_LANCZOS2 ||
        method == VS_SCALINGMETHOD_SPLINE36_FAST ||
        method == VS_SCALINGMETHOD_LANCZOS3_FAST ||
        method == VS_SCALINGMETHOD_SPLINE36 ||
        method == VS_SCALINGMETHOD_LANCZOS3)
    {
      // if scaling is below level, avoid hq scaling
      const float scaleX = fabs((static_cast<float>(m_sourceWidth) - m_viewWidth) / m_sourceWidth) * 100;
      const float scaleY = fabs((static_cast<float>(m_sourceHeight) - m_viewHeight) / m_sourceHeight) * 100;
      const int minScale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_HQSCALERS);

      return scaleX >= minScale || scaleY >= minScale;
    }
  }
  return false;
}

void CRendererHQ::OnOutputReset()
{
  // re-create shader if output shader changed
  m_scalerShader.reset();
}

void CRendererHQ::SelectPSVideoFilter()
{
  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_CUBIC_MITCHELL:
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_SPLINE36_FAST:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
  case VS_SCALINGMETHOD_SPLINE36:
  case VS_SCALINGMETHOD_LANCZOS3:
    m_bUseHQScaler = true;
    break;
  default:
    m_bUseHQScaler = false;
    break;
  }

  if (m_scalingMethod == VS_SCALINGMETHOD_AUTO)
  {
    const bool scaleSD = m_sourceHeight < 720 && m_sourceWidth < 1280;
    const bool scaleUp = m_sourceHeight < m_viewHeight && m_sourceWidth < m_viewWidth;
    const bool scaleFps = m_fps < CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAutoScaleMaxFps + 0.01f;

    if (scaleSD && scaleUp && scaleFps && Supports(VS_SCALINGMETHOD_LANCZOS3_FAST))
    {
      m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
      m_bUseHQScaler = true;
    }
  }
  if (m_renderOrientation)
    m_bUseHQScaler = false;
}

bool CRendererHQ::HasHQScaler() const
{
  return m_bUseHQScaler && m_scalerShader;
}

void CRendererHQ::CheckVideoParameters()
{
  __super::CheckVideoParameters();

  if (m_scalingMethodGui != m_videoSettings.m_ScalingMethod)
  {
    m_scalingMethodGui = m_videoSettings.m_ScalingMethod;
    m_scalingMethod = m_scalingMethodGui;

    if (!Supports(m_scalingMethod))
    {
      CLog::LogF(LOGWARNING, "chosen scaling method {} is not supported by renderer",
                 static_cast<int>(m_scalingMethod));
      m_scalingMethod = VS_SCALINGMETHOD_AUTO;
    }

    SelectPSVideoFilter();
    m_scalerShader.reset();
  }
}

void CRendererHQ::UpdateVideoFilters()
{
  __super::UpdateVideoFilters();

  if (m_bUseHQScaler && !m_scalerShader)
  {
    m_intermediateState = {};
    // firstly try the more efficient two pass convolution shader
    m_scalerShader = std::make_unique<CConvolutionShaderSeparable>();

    if (!m_scalerShader->Create(m_scalingMethod, m_outputShader))
    {
      m_scalerShader.reset();
      CLog::LogF(LOGINFO, "two pass convolution shader init problem, falling back to one pass.");
    }

    // fallback on the one pass version
    if (!m_scalerShader)
    {
      m_scalerShader = std::make_unique<CConvolutionShader1Pass>();

      if (!m_scalerShader->Create(m_scalingMethod, m_outputShader))
      {
        // we are in a big trouble
        m_scalerShader.reset();
        m_bUseHQScaler = false;
        CGUIDialogKaiToast::QueueNotification(
            CGUIDialogKaiToast::Error,
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(34400),
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(34401));
      }
    }
  }
  if (m_scalerShader)
    m_scalerShader->SetFinalShader(true);
}

void CRendererHQ::FinalOutput(CD3DTexture& source, CD3DTexture& target, const CRect& sourceRect, const CPoint(&destPoints)[4])
{
  if (HasHQScaler())
  {
    auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
    const CRect destRect = context.StereoCorrection(CRect(destPoints[0], destPoints[2]));

    // the cache is only valid for the back buffer, the scaler binds its depth stencil view
    if (&target != &DX::Windowing()->GetBackBuffer())
    {
      m_scalerShader->Render(source, target, sourceRect, destRect, false);
      return;
    }

    const bool cacheFits = m_scaledCache.GetWidth() == target.GetWidth() &&
                           m_scaledCache.GetHeight() == target.GetHeight() &&
                           m_scaledCache.GetFormat() == target.GetFormat();
    if (!cacheFits)
    {
      m_scaledCache.Release();
      if (!m_scaledCache.Create(target.GetWidth(), target.GetHeight(), 1, D3D11_USAGE_DEFAULT,
                                target.GetFormat()))
      {
        m_scalerShader->Render(source, target, sourceRect, destRect, false);
        return;
      }
    }

    if (!m_reuseIntermediate || !cacheFits)
    {
      // the copy below is rounded to whole pixels, keep pixels outside the picture black
      float black[4] = {};
      DX::DeviceResources::Get()->ClearRenderTarget(m_scaledCache.GetRenderTarget(), black);
      m_scalerShader->Render(source, m_scaledCache, sourceRect, destRect, false);
    }

    CRect area = destRect;
    area.Intersect(context.StereoCorrection(context.GetScissors()));
    area.Intersect(CRect(0.0f, 0.0f, static_cast<float>(target.GetWidth()),
                         static_cast<float>(target.GetHeight())));
    if (area.IsEmpty())
      return;

    const D3D11_BOX box{
        static_cast<UINT>(std::floor(area.x1)), static_cast<UINT>(std::floor(area.y1)), 0,
        static_cast<UINT>(std::ceil(area.x2)),  static_cast<UINT>(std::ceil(area.y2)),  1};
    DX::DeviceResources::Get()->GetD3DContext()->CopySubresourceRegion(
        target.Get(), 0, box.left, box.top, 0, m_scaledCache.Get(), 0, &box);
  }
  else
  {
    CD3D11_VIEWPORT viewPort(0.f, 0.f, static_cast<float>(target.GetWidth()), static_cast<float>(target.GetHeight()));
    // restore view port
    DX::DeviceResources::Get()->GetD3DContext()->RSSetViewports(1, &viewPort);
    // restore scissors
    auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
    DX::Windowing()->SetScissors(context.StereoCorrection(context.GetScissors()));
    // render frame
    __super::FinalOutput(source, target, sourceRect, destPoints);
  }
}
