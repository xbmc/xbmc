/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirtyRegion.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <memory>
#include <optional>
#include <string>

class CGraphicContext;
class CGUIImage;

//! \brief Fills the part of the raster the interface leaves uncovered while it holds its
//! shape. The viewer's choice wins, then the skin's `<surround>`; an unpainted band is black.
class CGUISurroundRenderer
{
public:
  CGUISurroundRenderer();
  ~CGUISurroundRenderer();

  //! \brief Place the surround image and run its process phase, which applies its fit and
  //! polls a texture still loading. Call on every frame, before Render().
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyRegions);

  //! \brief Paint the surround: before the window, only in the leftover bands, under a clip
  //! lifted to the raster and no further.
  void Render();

  //! \brief Forget what the surround is painted with, so the next frame resolves it again.
  //! Call when a surround setting moves or the skin changes.
  void Invalidate();

  //! \brief Let go of the image and its texture handle. Call on GUI shutdown, before the
  //! texture system goes.
  void ReleaseResources();

private:
  //! \brief What the surround is painted with. Both can be in force.
  struct SurroundArt
  {
    std::optional<KODI::UTILS::COLOR::Color> colour;
    std::string image;

    bool Nothing() const { return !colour && image.empty(); }
  };

  //! \brief The art in force, resolved from the settings and the skin. Held because Render()
  //! runs on every frame the hold is up, and neither source moves without one of the two
  //! calls above.
  const SurroundArt& Art();

  //! \brief Fit the image to \p raster at its own ratio. \return whether that moved it
  bool FitImage(const CRect& raster);

  void RenderImage(const CRect (&bands)[4], CGraphicContext& context);

  //! \brief The image drawn in the surround, kept between frames.
  std::unique_ptr<CGUIImage> m_image;
  std::string m_imagePath;

  SurroundArt m_art;
  bool m_artResolved{false};
};
