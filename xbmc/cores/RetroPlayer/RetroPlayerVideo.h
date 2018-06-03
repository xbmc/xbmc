/*
 *      Copyright (C) 2012-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "games/addons/GameClientCallbacks.h"

class CPixelConverter;

namespace KODI
{
namespace RETRO
{
  class CRPProcessInfo;
  class CRPRenderManager;

  /*!
   * \brief Renders video frames provided by the game loop
   *
   * \sa CRPRenderManager
   */
  class CRetroPlayerVideo : public GAME::IGameVideoCallback
  {
  public:
    CRetroPlayerVideo(CRPRenderManager& m_renderManager, CRPProcessInfo& m_processInfo);

    ~CRetroPlayerVideo() override;

    // implementation of IGameVideoCallback
    bool OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, unsigned int orientationDeg) override;
    bool OpenEncodedStream(AVCodecID codec) override;
    void AddData(const uint8_t* data, size_t size) override;
    void CloseStream() override;

  private:
    // Construction parameters
    CRPRenderManager& m_renderManager;
    CRPProcessInfo&   m_processInfo;
  };
}
}
