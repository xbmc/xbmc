/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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

  class CRetroPlayerVideo : public GAME::IGameVideoCallback
  {
  public:
    CRetroPlayerVideo(CRPRenderManager& m_renderManager, CRPProcessInfo& m_processInfo);

    ~CRetroPlayerVideo() override;

    // implementation of IGameVideoCallback
    bool OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, unsigned int orientationDeg) override;
    bool OpenEncodedStream(AVCodecID codec) override;
    void AddData(const uint8_t* data, unsigned int size) override;
    void CloseStream() override;

  private:
    // Construction parameters
    CRPRenderManager& m_renderManager;
    CRPProcessInfo&   m_processInfo;
  };
}
}
