/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISurroundRenderer.h"

#include "GUIComponent.h"
#include "GUIImage.h"
#include "GUITexture.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ColorUtils.h"
#include "video/geometry/GeometryTransforms.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

namespace
{

void PaintBands(const CRect (&bands)[4], KODI::UTILS::COLOR::Color colour)
{
  for (const CRect& band : bands)
  {
    if (!band.IsEmpty())
      CGUITexture::DrawQuad(band, colour);
  }
}

} // unnamed namespace

CGUISurroundRenderer::CGUISurroundRenderer() = default;

CGUISurroundRenderer::~CGUISurroundRenderer() = default;

void CGUISurroundRenderer::Invalidate()
{
  m_artResolved = false;
}

const CGUISurroundRenderer::SurroundArt& CGUISurroundRenderer::Art()
{
  if (m_artResolved)
    return m_art;

  m_art = {};

  const auto settings = CServiceBroker::GetSettingsComponent();
  const auto values = settings ? settings->GetSettings() : nullptr;
  if (!values)
    return m_art; // nothing to resolve from yet, so try again rather than hold the emptiness

  m_artResolved = true;

  switch (values->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISURROUND))
  {
    case 1:
      m_art.colour = values->GetString(CSettings::SETTING_VIDEOSCREEN_GUISURROUNDCOLOUR);
      break;
    case 2:
      m_art.image = values->GetString(CSettings::SETTING_VIDEOSCREEN_GUISURROUNDIMAGE);
      break;
    case 3:
      break;
    default:
      if (const auto skin = CServiceBroker::GetGUI()->GetSkinInfo())
      {
        m_art.colour = skin->GetSurroundColour();
        m_art.image = skin->GetSurroundImage();
      }
      break;
  }

  return m_art;
}

void CGUISurroundRenderer::Render()
{
  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();

  const CRect held = context.GetGuiKeepShapeRect();
  if (held.IsEmpty())
    return;

  const SurroundArt& art = Art();
  if (art.Nothing())
  {
    ReleaseResources();
    return;
  }

  const CRect raster = context.GetRasterRect();
  const CRect bands[4] = {{raster.x1, raster.y1, held.x1, raster.y2},
                          {held.x2, raster.y1, raster.x2, raster.y2},
                          {held.x1, raster.y1, held.x2, held.y1},
                          {held.x1, held.y2, held.x2, raster.y2}};

  const CRect previousClip = context.SetClip(raster);
  context.SetTransform(TransformMatrix());

  if (!art.colour.empty())
    PaintBands(bands, PaintColour(art.colour));

  if (!art.image.empty())
    RenderImage(art.image, raster, bands, context);
  else
    ReleaseResources();

  context.RemoveTransform();
  context.SetClip(previousClip);
}

KODI::UTILS::COLOR::Color CGUISurroundRenderer::PaintColour(const std::string& colour)
{
  if (colour != m_colourText)
  {
    m_colourText = colour;
    m_colour = KODI::UTILS::COLOR::ConvertHexToColor(colour);
  }

  return m_colour;
}

void CGUISurroundRenderer::RenderImage(const std::string& image,
                                       const CRect& raster,
                                       const CRect (&bands)[4],
                                       CGraphicContext& context)
{
  if (!m_image || m_imagePath != image)
  {
    ReleaseResources();
    m_image = std::make_unique<CGUIImage>(0, 0, raster.x1, raster.y1, raster.Width(),
                                          raster.Height(), CTextureInfo(image));
    m_imagePath = image;
    m_image->AllocResources();
  }

  // Fitted at its own ratio, never stretched.
  CRect fitted = raster;
  const float textureHeight = m_image->GetTextureHeight();
  if (textureHeight > 0.0f)
    fitted = KODI::VIDEO::GEOMETRY::FitAspect(m_image->GetTextureWidth() / textureHeight, raster);
  m_image->SetPosition(fitted.x1, fitted.y1);
  m_image->SetWidth(fitted.Width());
  m_image->SetHeight(fitted.Height());

  for (const CRect& band : bands)
  {
    if (!band.IsEmpty() && context.SetClipRegion(band.x1, band.y1, band.Width(), band.Height()))
    {
      m_image->Render();
      context.RestoreClipRegion();
    }
  }
}

void CGUISurroundRenderer::ReleaseResources()
{
  if (m_image)
    m_image->FreeResources();

  m_image.reset();
  m_imagePath.clear();
}
