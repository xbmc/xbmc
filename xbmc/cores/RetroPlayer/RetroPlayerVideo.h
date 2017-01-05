/*
 *      Copyright (C) 2012-2016 Team Kodi
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
//#include "threads/Thread.h"

#include <memory>

class CDVDClock;
class CPixelConverter;
class CProcessInfo;
class CRenderManager;
struct DVDVideoPicture;

namespace GAME
{
  class CRetroPlayerVideo : public IGameVideoCallback
                            //protected CThread
  {
  public:
    CRetroPlayerVideo(CRenderManager& m_renderManager, CProcessInfo& m_processInfo);

    virtual ~CRetroPlayerVideo();

    // implementation of IGameVideoCallback
    virtual bool OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, double framerate, unsigned int orientationDeg) override;
    virtual bool OpenEncodedStream(AVCodecID codec) override;
    virtual void AddData(const uint8_t* data, unsigned int size) override;
    virtual void CloseStream() override;

    /*
  protected:
    // implementation of CThread
    virtual void Process(void);
    */

  private:
    bool Configure(DVDVideoPicture& picture);
    bool GetPicture(const uint8_t* data, unsigned int size, DVDVideoPicture& picture);
    void SendPicture(DVDVideoPicture& picture);

    // Construction parameters
    CRenderManager& m_renderManager;
    CProcessInfo&   m_processInfo;

    // Stream properties
    double       m_framerate;
    unsigned int m_orientation; // Degrees counter-clockwise
    bool         m_bConfigured; // Need first picture to configure the render manager
    unsigned int m_droppedFrames;
    std::unique_ptr<CPixelConverter> m_pixelConverter;
  };
}
