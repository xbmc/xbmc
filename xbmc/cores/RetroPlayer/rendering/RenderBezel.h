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
#include <stdint.h>
#include <string>

class CTexture;
class CTextureCache;

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Artwork drawn over the game to frame it, with a window the game shows through
 *
 * A bezel is made for a widescreen TV and a game of another shape, usually
 * 4:3. Where its window sits varies from one picture to the next - a picture
 * of an arcade cabinet puts it wherever the cabinet's screen is - so the
 * window is found from the picture itself.
 *
 * FindGameWindow() finds the window once, when the picture loads. The picture is
 * then drawn as it is, so any glass and the frame's soft inner edge lie over
 * the game as the artist drew them.
 *
 * Finding the window this way assumes the picture's centre is inside it. The
 * run of opaque pixels that starts the frame is how Kodi reads the picture,
 * not something the artwork has to be made for.
 */
class CRenderBezel
{
public:
  ~CRenderBezel();

  /*!
   * \brief Load a bezel, caching its picture as other artwork is cached
   *
   * This blocks while the picture downloads, so it belongs off the render
   * thread.
   *
   * \param url The bezel's picture
   * \param textureCache The cache the picture is kept in
   *
   * \return The bezel, or nullptr if the picture didn't load or has no window
   */
  static std::unique_ptr<CRenderBezel> Load(const std::string& url, CTextureCache& textureCache);

  /*!
   * \brief The part of the screen the bezel covers, kept to its own shape
   */
  CRect GetBezelRect(const CRect& screen) const;

  /*!
   * \brief The part of the screen the game shows through
   */
  CRect GetWindowRect(const CRect& screen) const;

  /*!
   * \brief Draw the bezel over the game
   *
   * Must be called on the render thread.
   */
  void Render(const CRect& screen);

  /*!
   * \brief Find the window in a picture
   *
   * The window is the run of the middle row and column, either side of the
   * centre, before the frame. The frame starts at the first few fully opaque
   * pixels in a row, or at any fully opaque pixels left at the edge of the
   * picture, so glass and the frame's soft inner edge are window.
   *
   * \param pixels 32-bit pixels, alpha in the fourth byte
   * \param width The picture's width
   * \param height The picture's height
   * \param pitch The bytes from one row to the next
   * \param[out] window The window, in pixels
   *
   * \return True if the picture has a window at its centre, false otherwise
   */
  static bool FindGameWindow(const uint8_t* pixels,
                             unsigned int width,
                             unsigned int height,
                             unsigned int pitch,
                             CRect& window);

  /*!
   * \brief True if the game, drawn inside the window, is framed well enough to
   *        show the bezel
   *
   * The game must fit in the window, cover most of it, and not shrink much to
   * do so. A small window, such as a photo of a cabinet with its screen in the
   * middle, a window of the wrong shape for the game, or a game stretched to
   * fill the screen, is not shown.
   *
   * \param window The window on screen
   * \param gameInWindow Where the game is drawn inside the window
   * \param gameOnScreen Where the game is drawn without a bezel
   */
  static bool FramesGame(const CRect& window, const CRect& gameInWindow, const CRect& gameOnScreen);

private:
  CRenderBezel(std::unique_ptr<CTexture> texture, const CRect& window);

  const std::unique_ptr<CTexture> m_texture;
  const unsigned int m_width;
  const unsigned int m_height;
  const CRect m_window;
};
} // namespace RETRO
} // namespace KODI
