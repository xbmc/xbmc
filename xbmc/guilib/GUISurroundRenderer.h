/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <memory>
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

  //! \brief Paint the surround: before the window, only in the leftover bands, under a clip
  //! lifted to the raster and no further.
  void Render();

  //! \brief Forget what the surround is painted with, so the next frame resolves it again.
  //! Call when a surround setting moves; a skin change arrives via ReleaseResources().
  void Invalidate();

  //! \brief Let go of the image and its texture handle. Call on GUI shutdown, before the
  //! texture system goes.
  void ReleaseResources();

private:
  //! \brief What the surround is painted with. Both can be in force.
  struct SurroundArt
  {
    std::string colour;
    std::string image;

    bool Nothing() const { return colour.empty() && image.empty(); }
  };

  //! \brief The art in force, resolved from the settings and the skin. Held because Render()
  //! runs on every frame the hold is up, and neither source moves without one of the two
  //! calls above.
  const SurroundArt& Art();

  void RenderImage(const std::string& image,
                   const CRect& raster,
                   const CRect (&bands)[4],
                   CGraphicContext& context);

  //! \brief The colour in force, parsed, held beside the text it came from.
  KODI::UTILS::COLOR::Color PaintColour(const std::string& colour);

  //! \brief The image drawn in the surround, kept between frames.
  std::unique_ptr<CGUIImage> m_image;
  std::string m_imagePath;

  std::string m_colourText;
  KODI::UTILS::COLOR::Color m_colour{0};

  SurroundArt m_art;
  bool m_artResolved{false};
};
