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

#include "system_gl.h"

#include "DllAvCodec.h"
#include "DVDVideoCodecFFmpeg.h"
#include <libavcodec/vaapi.h>
#include <va/va.h>
#include <list>
#include <boost/shared_ptr.hpp>

#ifdef HAVE_VA_X11
# include <va/va_x11.h>
#endif
#ifdef HAVE_VA_GLX
# include <va/va_glx.h>
#endif
#ifdef HAVE_VA_EGL
# include <va/va_egl.h>
#endif
#ifdef HAS_EGL
# include <EGL/egl.h>
# include <EGL/eglext.h>
#endif

namespace VAAPI {

typedef boost::shared_ptr<VASurfaceID const> VASurfacePtr;

struct CDisplay
  : CCriticalSection
{
  CDisplay(VADisplay display, bool deinterlace)
    : m_display(display)
    , m_lost(false)
    , m_deinterlace(deinterlace)
  {}
 ~CDisplay();

  VADisplay get() { return m_display; }
  bool      lost()          { return m_lost; }
  void      lost(bool lost) { m_lost = lost; }
  bool      support_deinterlace() { return m_deinterlace; };
private:
  VADisplay m_display;
  bool      m_lost;
  bool      m_deinterlace;
};

typedef boost::shared_ptr<CDisplay> CDisplayPtr;

struct CSurface
{
  CSurface(VASurfaceID id, CDisplayPtr& display)
   : m_id(id)
   , m_display(display)
  {}

 ~CSurface();

  VASurfaceID m_id;
  int         m_width;
  int         m_height;
  CDisplayPtr m_display;
};

typedef boost::shared_ptr<CSurface> CSurfacePtr;

struct CSurfaceGL
{
  CSurfaceGL(CDisplayPtr& display)
    : m_display(display)
  {}
  virtual ~CSurfaceGL() = 0;

  CDisplayPtr m_display;
};

#ifdef HAVE_VA_GLX
struct CSurfaceGLX : public CSurfaceGL
{
  CSurfaceGLX(CDisplayPtr& display, void *surface)
    : CSurfaceGL(display)
    , m_id(surface)
  {}
  virtual ~CSurfaceGLX();

  void*       m_id;
};

typedef boost::shared_ptr<CSurfaceGLX> CSurfaceGLXPtr;
#endif

#ifdef HAS_EGL
struct CSurfaceEGL : public CSurfaceGL
{
                CSurfaceEGL(CDisplayPtr& display);
  virtual      ~CSurfaceEGL() = 0;

  static const unsigned int kMaxPlanes = 3;

  virtual bool  Upload(CSurfacePtr surface, unsigned int flags) = 0;
  virtual bool  EnsureSize(int width, int height) = 0;

  EGLDisplay    m_eglDisplay;
  GLenum        m_format;
  unsigned int  m_numPlanes;
  EGLImageKHR   m_images[kMaxPlanes];

protected:
  void          DestroyImage(unsigned int index);
  void          DestroyImages();
};

typedef boost::shared_ptr<CSurfaceEGL> CSurfaceEGLPtr;

#ifdef HAVE_VA_X11
struct CSurfaceEGLPixmap : public CSurfaceEGL
{
                CSurfaceEGLPixmap(CDisplayPtr& display);
  virtual      ~CSurfaceEGLPixmap();

  virtual bool  Upload(CSurfacePtr surface, unsigned int flags);
  virtual bool  EnsureSize(int width, int height);

  static CSurfaceEGL *Create(CDisplayPtr& display, int width, int height);

  Display*      m_display;
  Pixmap        m_pixmap;
  int           m_pixmapWidth;
  int           m_pixmapHeight;
};
#endif

#ifdef HAVE_VA_EGL
struct CSurfaceEGLBuffer : public CSurfaceEGL
{
                CSurfaceEGLBuffer(CDisplayPtr& display);
  virtual      ~CSurfaceEGLBuffer();

  virtual bool  Upload(CSurfacePtr surface, unsigned int flags);
  virtual bool  EnsureSize(int width, int height);

  static CSurfaceEGL *Create(CDisplayPtr& display, int width, int height);

  CSurfacePtr   m_surface;
  void*         m_surfaceBuffer;
  int           m_surfaceWidth;
  int           m_surfaceHeight;
};
#endif
#endif

// silly type to avoid includes
struct CHolder
{
  CDisplayPtr    display;
  CSurfacePtr    surface;
#ifdef HAVE_VA_GLX
  CSurfaceGLXPtr surfglx;
#endif
#ifdef HAS_EGL
  CSurfaceEGLPtr surfegl;
#endif

  CHolder()
  {}
};

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
{
  bool EnsureContext(AVCodecContext *avctx);
  bool EnsureSurfaces(AVCodecContext *avctx, unsigned n_surfaces_count);
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces = 0);
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

  int                    m_refs;
  std::list<CSurfacePtr> m_surfaces_used;
  std::list<CSurfacePtr> m_surfaces_free;

  CDisplayPtr    m_display;
  VAConfigID     m_config;
  VAContextID    m_context;

  vaapi_context *m_hwaccel;

  CHolder        m_holder; // silly struct to pass data to renderer
};

}
