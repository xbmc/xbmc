/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "Codecs/DllAvCodec.h"
#include "DVDVideoCodecFFmpeg.h"
#include <libavcodec/vaapi.h>
#include <va/va.h>
#include <va/va_x11.h>
#include <va/va_glx.h>
#include <list>
#include <boost/shared_ptr.hpp>


namespace VAAPI {

typedef boost::shared_ptr<VASurfaceID const> VASurfacePtr;

struct CDisplay
  : CCriticalSection
{
  CDisplay(VADisplay display)
    : m_display(display)
  {}
 ~CDisplay();

  VADisplay get() { return m_display; }

private:
  VADisplay m_display;
};

typedef boost::shared_ptr<CDisplay> CDisplayPtr;

struct CSurface
{
  CSurface(VASurfaceID id, CDisplayPtr display)
   : m_id(id)
   , m_display(display)
  {}

 ~CSurface();

  VASurfaceID m_id;
  CDisplayPtr m_display;
};

typedef boost::shared_ptr<CSurface> CSurfacePtr;

// silly type to avoid includes
struct CHolder
{
  CDisplayPtr display;
  CSurfacePtr surface;
  void*       surfacegl;

  CHolder()
   : surfacegl(NULL)
  {}
};

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
{
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual int  Check     (AVCodecContext* avctx);
  virtual void Close();
  virtual const std::string Name() { return "vaapi"; }
  virtual CCriticalSection* Section() { if(m_display) return m_display.get(); else return NULL; }

  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void  RelBuffer(AVCodecContext *avctx, AVFrame *pic);

  VADisplay    GetDisplay() { return m_display->get(); }
protected:
  
  static const unsigned  m_surfaces_max = 32;
  unsigned               m_surfaces_count;
  VASurfaceID            m_surfaces[m_surfaces_max];

  std::list<CSurfacePtr> m_surfaces_used;
  std::list<CSurfacePtr> m_surfaces_free;

  CDisplayPtr    m_display;
  VAConfigID     m_config;
  VAContextID    m_context;

  vaapi_context *m_hwaccel;

  CHolder        m_holder; // silly struct to pass data to renderer
};

}
