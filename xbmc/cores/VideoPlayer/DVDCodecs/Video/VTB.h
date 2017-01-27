/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "system_gl.h"

#include "DVDVideoCodecFFmpeg.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

class CProcessInfo;

namespace VTB
{

class CDecoder: public IHardwareDecoder
{
public:
  CDecoder(CProcessInfo& processInfo);
 ~CDecoder();
  virtual bool Open(AVCodecContext* avctx, AVCodecContext* mainctx,
                    const enum AVPixelFormat, unsigned int surfaces = 0) override;
  virtual CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  virtual bool GetPicture(AVCodecContext* avctx, DVDVideoPicture* picture) override;
  virtual CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  virtual const std::string Name() override { return "vtb"; }
  virtual unsigned GetAllowedReferences() override ;

  void Close();

protected:
  unsigned m_renderbuffers_count;
  AVCodecContext *m_avctx;
  CProcessInfo& m_processInfo;
  struct __CVBuffer *m_renderPicture;
};

}
