/*
 *      Copyright (C) 2005-2013 Team XBMC
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

struct AVVDAContext;
class CBitstreamConverter;

namespace VDA
{

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
{
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum PixelFormat, unsigned int surfaces = 0);
  virtual int Decode(AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual int Check(AVCodecContext* avctx);
  virtual void Close();
  virtual const std::string Name() { return "vda"; }
  virtual unsigned GetAllowedReferences();

protected:
  bool Create(AVCodecContext* avctx);
  unsigned m_renderbuffers_count;
  struct AVVDAContext* m_ctx;
  CBitstreamConverter *m_bitstream;
};

}
