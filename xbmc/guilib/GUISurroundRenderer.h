/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <memory>
#include <string>

class CGraphicContext;
class CGUIImage;

//! \brief Fills the part of the raster the interface leaves uncovered while it holds its shape.
//! The viewer's choice wins, then the skin's own `<surround>`; an unpainted band is black.
class CGUISurroundRenderer
{
public:
  CGUISurroundRenderer();
  ~CGUISurroundRenderer();

  //! \brief Paint the surround for this pass: before the window, only in the leftover bands, and
  //! under a clip lifted to the raster and no further.
  void Render();

  //! \brief Let go of the image and the texture handle it holds. Called on GUI shutdown - the
  //! texture system is gone by the time the destructor runs.
  void ReleaseResources();

private:
  void RenderImage(const std::string& image,
                   const CRect& raster,
                   const CRect (&bands)[4],
                   CGraphicContext& context);

  //! \brief The image drawn in the surround, kept between frames so its texture is not reloaded.
  std::unique_ptr<CGUIImage> m_image;
  std::string m_imagePath;
};
