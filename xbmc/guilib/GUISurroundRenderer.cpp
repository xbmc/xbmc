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

//! \brief What the surround is painted with. A colour and an image can both be in force: a
//! skin may declare both, and the image is fitted rather than stretched, so the colour shows
//! wherever the fit leaves off.
struct SurroundArt
{
  std::string colour;
  std::string image;
};

SurroundArt ResolveSurroundArt(const CSettings& settings)
{
  SurroundArt art;
  switch (settings.GetInt(CSettings::SETTING_VIDEOSCREEN_GUISURROUND))
  {
    case 1:
      art.colour = settings.GetString(CSettings::SETTING_VIDEOSCREEN_GUISURROUNDCOLOUR);
      break;
    case 2:
      art.image = settings.GetString(CSettings::SETTING_VIDEOSCREEN_GUISURROUNDIMAGE);
      break;
    default:
      if (const auto skin = CServiceBroker::GetGUI()->GetSkinInfo())
      {
        art.colour = skin->GetSurroundColour();
        art.image = skin->GetSurroundImage();
      }
      break;
  }
  return art;
}

void PaintBands(const CRect (&bands)[4], const std::string& colour)
{
  const KODI::UTILS::COLOR::Color quadColour = KODI::UTILS::COLOR::ConvertHexToColor(colour);
  for (const CRect& band : bands)
  {
    if (!band.IsEmpty())
      CGUITexture::DrawQuad(band, quadColour);
  }
}

} // unnamed namespace

CGUISurroundRenderer::CGUISurroundRenderer() = default;

CGUISurroundRenderer::~CGUISurroundRenderer() = default;

void CGUISurroundRenderer::Render()
{
  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();

  const CRect held = context.GetGuiKeepShapeRect();
  if (held.IsEmpty())
    return;

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings)
    return;

  const SurroundArt art = ResolveSurroundArt(*settings);
  if (art.colour.empty() && art.image.empty())
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
    PaintBands(bands, art.colour);

  if (!art.image.empty())
    RenderImage(art.image, raster, bands, context);
  else
    ReleaseResources();

  context.RemoveTransform();
  context.SetClip(previousClip);
}

void CGUISurroundRenderer::RenderImage(const std::string& image,
                                       const CRect& raster,
                                       const CRect (&bands)[4],
                                       CGraphicContext& context)
{
  // Held across frames, texture and all: this runs per rendered frame, and freeing the texture
  // at the end of one would put the load path back in front of the next.
  if (!m_image || m_imagePath != image)
  {
    ReleaseResources();
    m_image = std::make_unique<CGUIImage>(0, 0, raster.x1, raster.y1, raster.Width(),
                                          raster.Height(), CTextureInfo(image));
    m_imagePath = image;
    m_image->AllocResources();
  }

  // Fitted at its own ratio, never stretched. Computed here in screen pixels rather than left to
  // the control's KEEP mode, which compensates for the rendering resolution's pixel ratio.
  CRect fitted = raster;
  const float textureHeight = m_image->GetTextureHeight();
  if (textureHeight > 0.0f)
    fitted = KODI::VIDEO::GEOMETRY::FitAspect(m_image->GetTextureWidth() / textureHeight, raster);
  m_image->SetPosition(fitted.x1, fitted.y1);
  m_image->SetWidth(fitted.Width());
  m_image->SetHeight(fitted.Height());

  // The image spans the raster and the interface is drawn over its middle, but a skin is
  // not obliged to be opaque, so only the bands are actually painted.
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
